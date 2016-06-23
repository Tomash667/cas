#include "Pch.h"
#include "Base.h"
#include "Run.h"
#include "Function.h"
#include "Op.h"

enum REF_TYPE
{
	REF_GLOBAL,
	REF_LOCAL,
	REF_ARG
};

enum SPECIAL_VAR
{
	V_REF,
	V_FUNCTION
};

vector<Var> stack, global, local;
vector<uint> expected_stack;
int current_function, args_offset, locals_offset;

void ExecuteFunction(Function& f)
{
	assert(f.arg_infos.size() < 15u);
	int packedArgs[16];
	int packed = 0;
	Str* str_ret = nullptr;

	// verify and pack args
	assert(stack.size() >= f.arg_infos.size());
	for(uint i = 0; i < f.arg_infos.size(); ++i)
	{
		Var& v = stack.at(stack.size() - f.arg_infos.size() + i);
		assert(v.type == f.arg_infos[i].type);
		int value;
		switch(v.type)
		{
		case V_INT:
		case V_BOOL:
		case V_FLOAT:
			value = v.value;
			break;
		case V_STRING:
			value = (int)&v.str->s;
			break;
		default:
			assert(0);
			break;
		}
		packedArgs[packed++] = value;
	}

	// string return value
	if(f.result == V_STRING)
	{
		str_ret = Str::Get();
		str_ret->refs = 1;
		packedArgs[packed++] = (int)(&str_ret->s);
	}

	// call
	void* clbk = f.clbk;
	int result;
	float fresult;
	uint packedSize = packed * 4;
	int* args = &packedArgs[0];
	__asm
	{
		push ecx;

		// copy args
		mov ecx, packedSize;
		mov eax, args;
		add eax, ecx;
		cmp ecx, 0;
		je endcopy;
	copyloop:
		sub eax, 4;
		push dword ptr[eax];
		sub ecx, 4;
		jne copyloop;
	endcopy:

		// call
		call clbk;

		// get result
		add esp, packedSize;
		mov result, eax;
		fstp fresult;
		pop ecx;
		
		// 64 bit type
		//lea  ecx, retQW
		//mov  [ecx], eax
		//mov  4[ecx], edx
	};

	// update stack
	for(uint i = 0; i < f.arg_infos.size(); ++i)
	{
		Var& v = stack.back();
		if(v.type == V_STRING)
			v.str->Release();
		stack.pop_back();
	}

	// push result
	switch(f.result)
	{
	case V_VOID:
		break;
	case V_BOOL:
		stack.push_back(Var(result != 0));
		break;
	case V_INT:
		stack.push_back(Var(result));
		break;
	case V_FLOAT:
		stack.push_back(Var(fresult));
		break;
	case V_STRING:
		stack.push_back(Var(str_ret));
		break;
	default:
		assert(0);
		break;
	}
}

void RunCode(RunContext& ctx)
{
	stack.clear();
	global.resize(ctx.globals);
	local.clear();
	current_function = -1;

	int* start = ctx.code.data();
	int* end = start + ctx.code.size();
	int* c = start + ctx.entry_point;

	while(true)
	{
		Op op = (Op)*c++;
		switch(op)
		{
		case PUSH_TRUE:
			stack.push_back(Var(true));
			break;
		case PUSH_FALSE:
			stack.push_back(Var(false));
			break;
		case PUSH_INT:
			{
				int val = *c++;
				stack.push_back(Var(val));
			}
			break;
		case PUSH_FLOAT:
			{
				float val = *(float*)c;
				++c;
				stack.push_back(Var(val));
			}
			break;
		case PUSH_STRING:
			{
				uint str_index = *c++;
				assert(str_index < ctx.strs.size());
				Str* str = ctx.strs[str_index];
				str->refs++;
				stack.push_back(Var(str));
			}
			break;
		case PUSH_LOCAL:
			{
				uint local_index = *c++;
				assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
				assert(ctx.ufuncs[current_function].locals > local_index);
				Var& v = local[locals_offset + local_index];
				assert(v.type != V_VOID);
				if(v.type == V_STRING)
					v.str->refs++;
				stack.push_back(v);
			}
			break;
		case PUSH_LOCAL_REF:
			{
				uint local_index = *c++;
				assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
				assert(ctx.ufuncs[current_function].locals > local_index);
				Var& v = local[locals_offset + local_index];
				assert(v.type != V_VOID && v.type != V_REF && v.type != V_STRING);
				stack.push_back(Var(V_REF, local_index, REF_LOCAL));
			}
			break;
		case PUSH_GLOBAL:
			{
				uint global_index = *c++;
				assert(global_index < global.size());
				Var& v = global[global_index];
				assert(v.type != V_VOID);
				if(v.type == V_STRING)
					v.str->refs++;
				stack.push_back(v);
			}
			break;
		case PUSH_GLOBAL_REF:
			{
				uint global_index = *c++;
				assert(global_index < global.size());
				Var& v = global[global_index];
				assert(v.type != V_VOID && v.type != V_REF && v.type != V_STRING);
				stack.push_back(Var(V_REF, global_index, REF_GLOBAL));
			}
			break;
		case PUSH_ARG:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
				assert(ctx.ufuncs[current_function].args.size() > arg_index);
				Var& v = local[args_offset + arg_index];
				assert(v.type != V_VOID);
				if(v.type == V_STRING)
					v.str->refs++;
				stack.push_back(v);
			}
			break;
		case PUSH_ARG_REF:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
				assert(ctx.ufuncs[current_function].args.size() > arg_index);
				Var& v = local[args_offset + arg_index];
				assert(v.type != V_VOID && v.type != V_REF && v.type != V_STRING);
				stack.push_back(Var(V_REF, arg_index, REF_ARG));
			}
			break;
		case POP:
			{
				assert(!stack.empty());
				Var& v = stack.back();
				if(v.type == V_STRING)
					v.str->Release();
				stack.pop_back();
			}
			break;
		case SET_LOCAL:
			{
				uint local_index = *c++;
				assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
				assert(ctx.ufuncs[current_function].locals > local_index);
				assert(!stack.empty());
				Var& v = local[locals_offset + local_index];
				// free what was in variable previously
				if(v.type == V_STRING)
					v.str->Release();
				Var& s = stack.back();
				// incrase reference for new var
				if(s.type == V_STRING)
					s.str->refs++;
				v = s;
			}
			break;
		case SET_GLOBAL:
			{
				uint global_index = *c++;
				assert(global_index < global.size());
				assert(!stack.empty());
				Var& v = global[global_index];
				// free what was in variable previously
				if(v.type == V_STRING)
					v.str->Release();
				Var& s = stack.back();
				// incrase reference for new var
				if(s.type == V_STRING)
					s.str->refs++;
				v = s;
			}
			break;
		case SET_ARG:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
				assert(ctx.ufuncs[current_function].args.size() > arg_index);
				assert(!stack.empty());
				Var& v = local[args_offset + arg_index];
				// free what was in variable previously
				if(v.type == V_STRING)
					v.str->Release();
				Var& s = stack.back();
				// incrase reference for new var
				if(s.type == V_STRING)
					s.str->refs++;
				v = s;
			}
			break;
		case CAST:
			// allowed casts bool/int/float -> anything
			{
				VAR_TYPE type = (VAR_TYPE)*c++;
				assert(!stack.empty());
				Var& v = stack.back();
				assert(v.type == V_BOOL || v.type == V_INT || v.type == V_FLOAT);
				if(v.type == V_BOOL)
				{
					assert(type == V_INT || type == V_FLOAT || type == V_STRING);
					if(type == V_INT)
					{
						// bool -> int
						v.value = (v.bvalue ? 1 : 0);
						v.type = V_INT;
					}
					else if(type == V_FLOAT)
					{
						// bool -> float
						v.fvalue = (v.bvalue ? 1.f : 0.f);
						v.type = V_FLOAT;
					}
					else
					{
						// bool -> string
						Str* str = Str::Get();
						str->s = (v.bvalue ? "true" : "false");
						str->refs = 1;
						v.str = str;
						v.type = V_STRING;
					}
				}
				else if(v.type == V_INT)
				{
					assert(type == V_BOOL || type == V_FLOAT || type == V_STRING);
					if(type == V_BOOL)
					{
						// int -> bool
						v.bvalue = (v.value != 0);
						v.type = V_BOOL;
					}
					else if(type == V_FLOAT)
					{
						// int -> float
						v.fvalue = (float)v.value;
						v.type = V_FLOAT;
					}
					else
					{
						// int -> string
						Str* str = Str::Get();
						str->s = Format("%d", v.value);
						str->refs = 1;
						v.str = str;
						v.type = V_STRING;
					}
				}
				else
				{
					assert(type == V_BOOL || type == V_INT || type == V_STRING);
					if(type == V_BOOL)
					{
						// float -> bool
						v.bvalue = (v.fvalue != 0.f);
						v.type = V_BOOL;
					}
					else if(type == V_INT)
					{
						// float -> int
						v.value = (int)v.fvalue;
						v.type = V_INT;
					}
					else
					{
						// float -> string
						Str* str = Str::Get();
						str->s = Format("%g", v.fvalue);
						str->refs = 1;
						v.str = str;
						v.type = V_STRING;
					}
				}
			}
			break;
		case NEG:
		case NOT:
		case BIT_NOT:
		case PRE_INC:
		case PRE_DEC:
		case POST_INC:
		case POST_DEC:
		case DEREF:
			{
				assert(!stack.empty());
				Var& v = stack.back();
				if(op == NEG)
				{
					assert(v.type == V_INT || v.type == V_FLOAT);
					if(v.type == V_INT)
						v.value = -v.value;
					else
						v.fvalue = -v.fvalue;
				}
				else if(op == NOT)
				{
					assert(v.type == V_BOOL);
					v.bvalue = !v.bvalue;
				}
				else if(op == BIT_NOT)
				{
					assert(v.type == V_INT);
					v.value = ~v.value;
				}
				else if(op == DEREF)
				{
					assert(v.type == V_REF);
					Var* vr;
					if(v.value2 == REF_LOCAL)
					{
						assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
						assert(ctx.ufuncs[current_function].locals > (uint)v.value1);
						vr = &local[locals_offset + v.value1];
					}
					else if(v.value2 == REF_GLOBAL)
					{
						assert(global.size() > (uint)v.value1);
						vr = &global[v.value1];
					}
					else
					{
						assert(v.value2 == REF_ARG);
						assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
						assert(ctx.ufuncs[current_function].args.size() >(uint)v.value1);
						vr = &local[args_offset + v.value1];
					}
					v = *vr;
					if(v.type == V_STRING)
						v.str->refs++;
				}
				else
				{
					assert(v.type == V_REF);
					Var* vr;
					if(v.value2 == REF_LOCAL)
					{
						assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
						assert(ctx.ufuncs[current_function].locals >(uint)v.value1);
						vr = &local[locals_offset + v.value1];
					}
					else if(v.value2 == REF_GLOBAL)
					{
						assert(global.size() > (uint)v.value1);
						vr = &global[v.value1];
					}
					else
					{
						assert(v.value2 == REF_ARG);
						assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
						assert(ctx.ufuncs[current_function].args.size() >(uint)v.value1);
						vr = &local[args_offset + v.value1];
					}
					Var& rv = *vr;
					assert(rv.type == V_INT || rv.type == V_FLOAT);
					switch(op)
					{
					case PRE_INC:
						if(rv.type == V_INT)
							++rv.value;
						else
							++rv.fvalue;
						break;
					case PRE_DEC:
						if(rv.type == V_INT)
							--rv.value;
						else
							--rv.fvalue;
						break;
					case POST_INC:
						stack.pop_back();
						if(rv.type == V_INT)
							stack.push_back(Var(rv.value++));
						else
							stack.push_back(Var(rv.fvalue++));
						break;
					case POST_DEC:
						stack.pop_back();
						if(rv.type == V_INT)
							stack.push_back(Var(rv.value--));
						else
							stack.push_back(Var(rv.fvalue--));
						break;
					}
				}
			}
			break;
		case ADD:
		case SUB:
		case MUL:
		case DIV:
		case MOD:
		case EQ:
		case NOT_EQ:
		case GR:
		case GR_EQ:
		case LE:
		case LE_EQ:
		case AND:
		case OR:
		case BIT_AND:
		case BIT_OR:
		case BIT_XOR:
		case BIT_LSHIFT:
		case BIT_RSHIFT:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == right.type);
				if(op == ADD)
					assert(left.type == V_INT || left.type == V_FLOAT || left.type == V_STRING);
				else if(op == EQ || op == NOT_EQ)
					assert(left.type == V_BOOL || left.type == V_INT || left.type == V_FLOAT);
				else if(op == AND || op == OR)
					assert(left.type == V_BOOL);
				else if(op == BIT_AND || op == BIT_OR || op == BIT_XOR || op == BIT_LSHIFT || op == BIT_RSHIFT)
					assert(left.type == V_INT);
				else
					assert(left.type == V_INT || left.type == V_FLOAT);

				switch(op)
				{
				case ADD:
					if(left.type == V_INT)
						left.value += right.value;
					else if(left.type == V_FLOAT)
						left.fvalue += right.fvalue;
					else
					{
						string result = left.str->s + right.str->s;
						left.str->Release();
						right.str->Release();
						Str* s = Str::Get();
						s->refs = 1;
						s->s = result;
						left.str = s;
					}
					break;
				case SUB:
					if(left.type == V_INT)
						left.value -= right.value;
					else
						left.fvalue -= right.fvalue;
					break;
				case MUL:
					if(left.type == V_INT)
						left.value *= right.value;
					else
						left.fvalue *= right.fvalue;
					break;
				case DIV:
					if(left.type == V_INT)
					{
						if(right.value == 0)
							left.value = 0;
						else
							left.value /= right.value;
					}
					else
					{
						if(right.fvalue == 0.f)
							left.fvalue = 0.f;
						else
							left.fvalue /= right.fvalue;
					}
					break;
				case MOD:
					if(left.type == V_INT)
					{
						if(right.value == 0)
							left.value = 0;
						else
							left.value %= right.value;
					}
					else
					{
						if(right.fvalue == 0.f)
							left.fvalue = 0.f;
						else
							left.fvalue = fmod(left.fvalue, right.fvalue);
					}
					break;
				case EQ:
					if(left.type == V_BOOL)
						left.bvalue = (left.bvalue == right.bvalue);
					else if(left.type == V_INT)
						left.bvalue = (left.value == right.value);
					else
						left.bvalue = (left.fvalue == right.fvalue);
					left.type = V_BOOL;
					break;
				case NOT_EQ:
					if(left.type == V_BOOL)
						left.bvalue = (left.bvalue != right.bvalue);
					else if(left.type == V_INT)
						left.bvalue = (left.value != right.value);
					else
						left.bvalue = (left.fvalue != right.fvalue);
					left.type = V_BOOL;
					break;
				case GR:
					if(left.type == V_INT)
						left.bvalue = (left.value > right.value);
					else
						left.bvalue = (left.fvalue > right.fvalue);
					left.type = V_BOOL;
					break;
				case GR_EQ:
					if(left.type == V_INT)
						left.bvalue = (left.value >= right.value);
					else
						left.bvalue = (left.fvalue >= right.fvalue);
					left.type = V_BOOL;
					break;
				case LE:
					if(left.type == V_INT)
						left.bvalue = (left.value < right.value);
					else
						left.bvalue = (left.fvalue < right.fvalue);
					left.type = V_BOOL;
					break;
				case LE_EQ:
					if(left.type == V_INT)
						left.bvalue = (left.value <= right.value);
					else
						left.bvalue = (left.fvalue <= right.fvalue);
					left.type = V_BOOL;
					break;
				case AND:
					left.bvalue = (left.bvalue && right.bvalue);
					break;
				case OR:
					left.bvalue = (left.bvalue || right.bvalue);
					break;
				case BIT_AND:
					left.value = (left.value & right.value);
					break;
				case BIT_OR:
					left.value = (left.value | right.value);
					break;
				case BIT_XOR:
					left.value = (left.value ^ right.value);
					break;
				case BIT_LSHIFT:
					left.value = (left.value << right.value);
					break;
				case BIT_RSHIFT:
					left.value = (left.value >> right.value);
					break;
				}
			}
			break;
		case RET:
			if(current_function == -1)
			{
				assert(stack.empty());
				assert(local.empty());
				return;
			}
			else
			{
				assert((uint)current_function < ctx.ufuncs.size());
				UserFunction& f = ctx.ufuncs[current_function];
				uint to_pop = f.locals + f.GetArgs();
				assert(local.size() > to_pop);
				while(to_pop--)
				{
					Var& v = local.back();
					if(v.type == V_STRING)
						v.str->Release();
					local.pop_back();
				}
				Var& func_mark = local.back();
				assert(func_mark.type == V_FUNCTION);
				c = start + func_mark.value2;
				current_function = func_mark.value1;
				local.pop_back();
				if(current_function != -1)
				{
					// checking local stack
					assert((uint)current_function < ctx.ufuncs.size());
					UserFunction& f = ctx.ufuncs[current_function];
					uint count = 1 + f.locals + f.GetArgs();
					assert(local.size() >= count);
					assert((local.end() - count)->type == V_FUNCTION);
				}
				assert(expected_stack.back() == stack.size());
				if(f.result != V_VOID)
					assert(stack.back().type == f.result);
				expected_stack.pop_back();
			}
			break;
		case JMP:
			{
				uint offset = *c;
				c = start + offset;
				assert(c < end);
			}
			break;
		case TJMP:
		case FJMP:
			{
				uint offset = *c++;
				int* new_c = start + offset;
				assert(new_c < end);
				assert(!stack.empty());
				Var v = stack.back();
				stack.pop_back();
				assert(v.type == V_BOOL);
				bool ok = v.bvalue;
				if(op == FJMP)
					ok = !ok;
				if(ok)
					c = new_c;
			}
			break;
		case CALL:
			{
				uint f_idx = *c++;
				assert(f_idx < functions.size());
				Function& f = *functions[f_idx];
				ExecuteFunction(f);
			}
			break;
		case CALLU:
			{
				uint f_idx = *c++;
				assert(f_idx < ctx.ufuncs.size());
				UserFunction& f = ctx.ufuncs[f_idx];
				// push function to locals
				uint pos = c - start;
				local.push_back(Var(V_FUNCTION, current_function, pos));
				// handle args
				assert(stack.size() >= f.args.size());
				args_offset = local.size();
				local.resize(local.size() + f.GetArgs());
				for(uint i = 0, count = f.GetArgs(); i < count; ++i)
				{
					assert(f.args[count - 1 - i] == stack.back().type);
					local[args_offset + count - 1 - i] = stack.back();
					stack.pop_back();
				}
				// handle locals
				locals_offset = local.size();
				local.resize(local.size() + f.locals);
				// call
				uint expected = stack.size();
				if(f.result != V_VOID)
					++expected;
				expected_stack.push_back(expected);
				current_function = f_idx;
				c = start + f.pos;
			}
			break;
		default:
			assert(0);
			break;
		}
		assert(c < end);
	}
}
