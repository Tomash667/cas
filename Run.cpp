#include "Pch.h"
#include "Base.h"
#include "Run.h"
#include "Function.h"
#include "Op.h"

vector<Var> stack;
vector<Var> vars;
vector<Function> functions;

void RunCode(vector<int>& code, vector<string>& strs, uint n_vars)
{
	vars.resize(n_vars);

	int* c = code.data();
	int* end = c + code.size();
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
				assert(str_index < strs.size());
				Str* str = Str::Get();
				str->refs = 1;
				str->s = strs[str_index];
				stack.push_back(Var(str));
			}
			break;
		case PUSH_VAR:
			{
				uint var_index = *c++;
				assert(var_index < vars.size());
				Var& v = vars[var_index];
				assert(v.type != V_VOID);
				if(v.type == V_STRING)
					v.str->refs++;
				stack.push_back(v);
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
		case POP_VAR:
			{
				uint var_index = *c++;
				assert(var_index < vars.size());
				assert(!stack.empty());
				Var& v = vars[var_index];
				if(v.type == V_STRING)
					v.str->Release();
				v = stack.back();
				stack.pop_back();
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
				else
				{
					assert(v.type == V_BOOL);
					v.bvalue = !v.bvalue;
				}
			}
			break;
		case ADD:
		case SUB:
		case MUL:
		case DIV:
		case EQ:
		case NOT_EQ:
		case GR:
		case GR_EQ:
		case LE:
		case LE_EQ:
		case AND:
		case OR:
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
				}
			}
			break;
		case RET:
			assert(stack.empty());
			return;
		case JMP:
			{
				uint offset = *c++;
				c = code.data() + offset;
				assert(c < end);
			}
			break;
		case TJMP:
			{
				uint offset = *c++;
				int* new_c = code.data() + offset;
				assert(new_c < end);
				assert(!stack.empty());
				Var v = stack.back();
				stack.pop_back();
				assert(v.type == V_BOOL);
				if(!v.bvalue)
					c = new_c;
			}
			break;
		case CALL:
			{
				uint f_idx = *c++;
				assert(f_idx < functions.size());
				Function& f = functions[f_idx];
				assert(stack.size() >= f.args.size());
				uint expected_stack_size = stack.size() - f.args.size();
				if(f.result != V_VOID)
					++expected_stack_size;
				for(uint i = 0; i < f.args.size(); ++i)
					assert(stack.at(stack.size() - f.args.size() + i).type == f.args[i]);
				f.clbk();
				assert(expected_stack_size == stack.size());
			}
			break;
		default:
			assert(0);
			break;
		}
		assert(c < end);
	}
}
