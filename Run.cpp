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
		case PUSH_INT:
			{
				int val = *c++;
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
			{
				VAR_TYPE type = (VAR_TYPE)*c++;
				assert(type == V_STRING);
				assert(!stack.empty());
				Var& v = stack.back();
				assert(v.type == V_INT);
				Str* str = Str::Get();
				str->s = Format("%d", v.value);
				str->refs = 1;
				stack.back() = Var(str);
			}
			break;
		case NEG:
			{
				assert(!stack.empty());
				Var& v = stack.back();
				assert(v.type == V_INT);
				v.value = -v.value;
			}
			break;
		case ADD:
		case SUB:
		case MUL:
		case DIV:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				assert(right.type == V_INT);
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == V_INT);
				switch(op)
				{
				case ADD:
					left.value += right.value;
					break;
				case SUB:
					left.value -= right.value;
					break;
				case MUL:
					left.value *= right.value;
					break;
				case DIV:
					if(right.value == 0)
						left.value = 0;
					else
						left.value /= right.value;
					break;
				}
			}
			break;
		case RET:
			assert(stack.empty());
			return;
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
