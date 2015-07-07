#include "Tokenizer.h"
#include "Function.h"
#include "Stack.h"
#include "Op.h"
#include "Run.h"

_StrPool StrPool;
vector<Var> stack;
uint args_offset, args_count, locals_offset, locals_count;

template<typename T>
T get(byte*& c)
{
	T t = *(T*)c;
	c += sizeof(T);
	return t;
}

inline int get_int(byte*& c)
{
	return get<int>(c);
}

inline uint get_uint(byte*& c)
{
	return get<uint>(c);
}

inline float get_float(byte*& c)
{
	return get<float>(c);
}

void stack_get2(Var& a, Var& b)
{
	assert(stack.size() >= 2u);
	a = stack.back();
	stack.pop_back();
	b = stack.back();
	stack.pop_back();
	assert(a.type != LOCALS_MARKER && b.type != LOCALS_MARKER);
}

Var& stack_get2ret(Var& a)
{
	assert(stack.size() >= 2u);
	a = stack.back();
	stack.pop_back();
	assert(a.type != LOCALS_MARKER && stack.back().type != LOCALS_MARKER);
	return stack.back();
}

void run(byte* code, vector<Str*>& strs, vector<ScriptFunction>& func, uint entry_point)
{
	assert(entry_point < func.size());
	ScriptFunction& _main = func[entry_point];
	byte* c = code + _main.offset;
	// args
	args_offset = 0;
	args_count = _main.args;
	assert(stack.size() == args_count);
	// return address
	Var ra;
	ra.type = RETURN_ADDRESS;
	ra.v.i = -1;
	ra.prev_func = -1;
	ra.offset = 0;
	stack.push_back(ra);
	// locals
	locals_offset = stack.size();
	locals_count = _main.locals;
	if(locals_count > 0)
		stack.resize(stack.size() + locals_count, Var(VOID));
#ifdef _DEBUG
	stack.push_back(Var(LOCALS_MARKER));
#endif

	while(true)
	{
		Op op = (Op)*c;
		++c;
		switch(op)
		{
		case add:
			{
				Var right, left;
				stack_get2(right, left);
				assert(left.type == right.type && (left.type == INT || left.type == FLOAT || left.type == STR));
				if(left.type == INT)
					stack.push_back(Var(left.v.i + right.v.i));
				else if(left.type == FLOAT)
					stack.push_back(Var(left.v.f + right.v.f));
				else
				{
					g_tmp_string2 = left.v.str->s + right.v.str->s;
					--right.v.str->refs;
					if(right.v.str->refs == 0)
					{
						right.v.str->s = g_tmp_string2;
						right.v.str->refs = 1;
						--left.v.str->refs;
						if(left.v.str->refs == 0)
							StrPool.Free(left.v.str);
						stack.push_back(right);
					}
					else
					{
						--left.v.str->refs;
						if(left.v.str->refs == 0)
						{
							left.v.str->s = g_tmp_string2;
							left.v.str->refs = 1;
							stack.push_back(left);
						}
						else
							stack.push_back(Var(g_tmp_string2));
					}
				}
			}
			break;
		case sub:
			{
				Var right;
				Var& left = stack_get2ret(right);
				assert(left.type == right.type && (left.type == INT || left.type == FLOAT));
				if(left.type == INT)
					left.v.i -= right.v.i;
				else
					left.v.f -= right.v.f;
			}
			break;
		case mul:
			{
				Var right;
				Var& left = stack_get2ret(right);
				assert(left.type == right.type && (left.type == INT || left.type == FLOAT));
				if(left.type == INT)
					left.v.i *= right.v.i;
				else
					left.v.f *= right.v.f;
			}
			break;
		case o_div:
			{
				Var right;
				Var& left = stack_get2ret(right);
				assert(left.type == right.type && (left.type == INT || left.type == FLOAT));
				if(left.type == INT)
				{
					if(right.v.i == 0)
						throw "Division by zero!";
					left.v.i /= right.v.i;
				}
				else
				{
					if(right.v.f == 0)
						throw "Division by zero!";
					left.v.f /= right.v.f;
				}
			}
			break;
		case mod:
			{
				Var right;
				Var& left = stack_get2ret(right);
				assert(left.type == right.type && left.type == INT);
				if(right.v.i == 0)
					throw "Division by zero!";
				left.v.i %= right.v.i;
			}
			break;
		case neg:
			{
				assert(!stack.empty());
				Var& a = stack.back();
				assert(a.type == INT || a.type == FLOAT);
				if(a.type == FLOAT)
					a.v.f = -a.v.f;
				else
					a.v.i = -a.v.i;
			}
			break;
		case inc_pre:
		case inc_post:
		case dec_pre:
		case dec_post:
			{
				assert(!stack.empty());
				Var& a = stack.back();
				assert(a.type == REF);
				Var& v = stack[locals_offset + a.v.i];
				assert(v.type == INT || v.type == FLOAT || v.type == ARRAY);
				VarType type;
				VarValue* vv;

				if(v.type == ARRAY)
				{
					assert((v.subtype == INT || v.subtype == FLOAT) && a.offset >= 0 && a.offset < (int)v.v.arr->size());
					vv = &v.v.arr->at(a.offset);
					type = v.subtype;
				}
				else
				{
					vv = &v.v;
					type = v.type;
				}

				bool pre = (op == inc_pre || op == dec_pre);
				bool inc = (op == inc_pre || op == inc_post);

				if(!pre)
				{
					if(type == INT)
						a.v.i = vv->i;
					else
						a.v.f = vv->f;
				}

				if(inc)
				{
					if(type == INT)
						vv->i++;
					else
						vv->f++;
				}
				else
				{
					if(type == INT)
						vv->i--;
					else
						vv->f--;
				}

				if(pre)
				{
					if(type == INT)
						a.v.i = vv->i;
					else
						a.v.f = vv->f;
				}

				a.type = type;
			}
			break;
		case equal:
		case not_equal:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == right.type);
				bool result;
				if(left.type == INT)
					result = (left.v.i == right.v.i);
				else if(left.type == FLOAT)
					result = (left.v.f == right.v.f);
				else if(left.type == BOOL)
					result = (left.v.b == right.v.b);
				else if(left.type == STR)
				{
					result = (left.v.str->s == right.v.str->s);
					left.Clean();
					right.Clean();
				}
				else
				{
					assert(0);
					result = false;
				}
				if(op == not_equal)
					result = !result;
				left.type = BOOL;
				left.v.b = result;
			}
			break;
		case greater:
		case greater_equal:
		case less:
		case less_equal:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == right.type && (left.type == INT || left.type == FLOAT));
				bool result = false;
				switch(op)
				{
				case greater:
					if(left.type == INT)
						result = (left.v.i > right.v.i);
					else
						result = (left.v.f > right.v.f);
					break;
				case greater_equal:
					if(left.type == INT)
						result = (left.v.i >= right.v.i);
					else
						result = (left.v.f >= right.v.f);
					break;
				case less:
					if(left.type == INT)
						result = (left.v.i < right.v.i);
					else
						result = (left.v.f < right.v.f);
					break;
				case less_equal:
					if(left.type == INT)
						result = (left.v.i <= right.v.i);
					else
						result = (left.v.f <= right.v.f);
					break;
				}
				left.type = BOOL;
				left.v.b = result;
			}
			break;
		case and:
		case or:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == right.type && left.type == BOOL);
				bool result;
				if(op == and)
					result = (left.v.b && right.v.b);
				else
					result = (left.v.b || right.v.b);
				left.v.b = result;
				left.type = BOOL;
			}
			break;
		case cast:
			{
				byte type = *c++;
				assert(!stack.empty());
				Var& a = stack.back();
				switch(type)
				{
				case STR:
					if(a.type == INT)
					{
						cstring s = Format("%d", a.v.i);
						a.v.str = StrPool.Get();
						a.v.str->s = s;
					}
					else if(a.type == FLOAT)
					{
						cstring s = Format("%g", a.v.f);
						a.v.str = StrPool.Get();
						a.v.str->s = s;
					}
					else if(a.type == BOOL)
					{
						cstring s = (a.v.b ? "true" : "false");
						a.v.str = StrPool.Get();
						a.v.str->s = s;
					}
					else
						assert(0);
					a.type = STR;
					a.v.str->refs = 1;
					break;
				case INT:
					if(a.type == FLOAT)
					{
						a.v.i = (int)a.v.f;
						a.type = INT;
					}
					else
						assert(0);
					break;
				case FLOAT:
					if(a.type == INT)
					{
						a.v.f = (float)a.v.i;
						a.type = FLOAT;
					}
					else
						assert(0);
					break;
				case BOOL:
					if(a.type == INT)
						a.v.b = (a.v.i != 0);
					else if(a.type == FLOAT)
						a.v.i = (a.v.f != 0.f);
					else
						assert(0);
					a.type = BOOL;
					break;
				default:
					assert(0);
					break;
				}
			}
			break;
		case init_array:
			{
				byte b = *c++;
				assert(b < locals_count);
				byte type = *c++;
				uint size = get_uint(c);
				Var& l = stack[locals_offset + b];
				VarValue zero;
				zero.i = 0;
				l.type = ARRAY;
				l.subtype = (VarType)type;
				l.v.arr = new vector<VarValue>(size, zero);
			}
			break;
		case set_local:
			{
				byte b = *c++;
				assert(b < locals_count);
				Var& l = stack[locals_offset + b];
				l.Clean();
				l = stack.back();
				if(l.type == STR)
					l.v.str->refs++;
			}
			break;
		case set_local_index: // local[b] = stack
			{
				byte b = *c++;
				assert(b < locals_count);
				uint index = get_uint(c);
				Var& l = stack[locals_offset + b];
				assert(l.type == ARRAY && l.v.arr->size() > index);
				assert(!stack.empty());
				Var& a = stack.back();
				VarValue& e = l.v.arr->at(index);
				l.Clean(index);
				e.i = a.v.i;
				if(l.subtype == STR)
					e.str->refs++;
			}
			break;
		case set_local_indexvar: // local[stack1] = stack0; pop; pop; push stack1
			{
				byte b = *c++;
				assert(b < locals_count);
				Var& l = stack[locals_offset + b];
				assert(l.type == ARRAY);
				assert(stack.size() >= 2u);
				Var value = stack.back();
				stack.pop_back();
				Var index = stack.back();
				stack.pop_back();
				assert(index.type == INT);
				uint idx = (uint)index.v.i;
				assert(l.v.arr->size() > idx);
				VarValue& e = l.v.arr->at(idx);
				l.Clean(idx);
				e.i = value.v.i;
				if(l.subtype == STR)
					e.str->refs++;
				stack.push_back(value);
			}
			break;
		case push_local:
			{
				byte b = *c++;
				assert(b < locals_count);
				Var& l = stack[locals_offset + b];
				if(l.type == STR)
					l.v.str->refs++;
				stack.push_back(l);
			}
			break;
		case push_local_ref:
			{
				byte b = *c++;
				assert(b < locals_count);
				Var v;
				v.type = REF;
				v.v.i = b;
				stack.push_back(v);
			}
			break;
		case push_local_index:
			{
				byte b = *c++;
				assert(b < locals_count);
				Var& l = stack[locals_offset + b];
				assert(l.type == ARRAY);
				uint index = get_uint(c);
				assert(l.v.arr->size() > index);
				stack.push_back(Var(l.subtype, l.v.arr->at(index)));
			}
			break;
		case push_local_indexvar:
			{
				byte b = *c++;
				assert(b < locals_count);
				Var& l = stack[locals_offset + b];
				assert(l.type == ARRAY);
				assert(!stack.empty());
				Var& idx = stack.back();
				assert(idx.type == INT);
				uint index = (uint)idx.v.i;
				assert(l.v.arr->size() > index);
				stack.pop_back();
				stack.push_back(Var(l.subtype, l.v.arr->at(index)));
			}
			break;
		case push_local_index_ref:
			{
				byte b = *c++;
				assert(b < locals_count);
				Var& l = stack[locals_offset + b];
				assert(l.type == ARRAY);
				uint index = get_uint(c);
				assert(l.v.arr->size() > index);
				Var v;
				v.type = REF;
				v.v.i = b;
				v.offset = index;
				stack.push_back(v);
			}
			break;
		case push_local_indexvar_ref:
			{
				byte b = *c++;
				assert(b < locals_count);
				Var& l = stack[locals_offset + b];
				assert(l.type == ARRAY);
				assert(!stack.empty());
				Var& idx = stack.back();
				assert(idx.type == INT);
				uint index = (uint)idx.v.i;
				assert(l.v.arr->size() > index);
				stack.pop_back();
				Var v;
				v.type = REF;
				v.v.i = b;
				v.offset = index;
				stack.push_back(v);
			}
			break;
		case push_cstr:
			{
				byte b = *c++;
				assert(b < strs.size());
				strs[b]->refs++;
				Var v;
				v.type = STR;
				v.v.str = strs[b];
				stack.push_back(v);
			}
			break;
		case push_int:
			stack.push_back(Var(get_int(c)));
			break;
		case push_float:
			stack.push_back(Var(get_float(c)));
			break;
		case dup:
			assert(!stack.empty());
			assert(stack.back().type != LOCALS_MARKER);
			stack.push_back(stack.back().Copy());
			break;
		case pop:
			assert(!stack.empty());
			stack.back().Clean();
			stack.pop_back();
			break;
		case call:
			{
				byte b = *c++;
				assert(b < functions.size());
				FunctionInfo& f = functions[b];
#ifndef NDEBUG
				assert(stack.size() >= f.params_count);
				int i = 0;
				for(vector<Var>::reverse_iterator rit = stack.rbegin(), rend = stack.rend(); rit != rend; ++rit)
				{
					if(f.params_count == i)
						break;
					assert(rit->type == f.params[i]);
					++i;
				}
#endif
				f.ptr();
			}
			break;
		case jmp:
			{
				int pos = get_int(c);
				c = code + pos;
			}
			break;
		case jmp_if:
			{
				int pos = get_int(c);
				assert(!stack.empty());
				Var& a = stack.back();
				assert(a.type == BOOL);
				if(!a.v.b)
					c = code + pos;
				stack.pop_back();
			}
			break;
		case ret:
			// pop locals marker
#ifdef _DEBUG
			assert(stack.back().type == LOCALS_MARKER);
			stack.pop_back();
#endif
			// pop locals
			stack.resize(stack.size() - locals_count);
			// pop return address
			assert(stack.back().type == RETURN_ADDRESS);
			stack.pop_back();
			return;
		}
	}
}
