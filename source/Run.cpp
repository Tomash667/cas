#include "Pch.h"
#include "CasImpl.h"
#include "RunModule.h"
#include "Module.h"
#include "Op.h"
#include "Run.h"

static vector<Var> stack, global, local;
static Var tmpv;
static vector<uint> expected_stack;
static int current_function, args_offset, locals_offset;
static RunModule* run_module;

Str* CreateStr()
{
	Str* str = Str::Get();
	str->refs = 1;
	return str;
}

Str* CreateStr(cstring s)
{
	Str* str = Str::Get();
	str->s = s;
	str->refs = 1;
	return str;
}

void AddRef(Var& v)
{
	assert(v.type != V_VOID);
	if(v.type == V_STRING)
		v.str->refs++;
	else if(v.type == V_REF)
	{
		if(v.ref_type == REF_MEMBER)
			v.ref_class->refs++;
	}
	else
	{
		Type* type = run_module->GetType(v.type);
		if(type->IsClass())
			v.clas->refs++;
	}
}

void ReleaseRef(Var& v)
{
	if(v.type == V_STRING)
		v.str->Release();
	else if(v.type == V_REF)
	{
		if(v.ref_type == REF_MEMBER)
			v.ref_class->Release();
	}
	else
	{
		Type* type = run_module->GetType(v.type);
		if(type->IsClass())
			v.clas->Release();
	}
}

struct GetRefData
{
	int* data;
	int type;

	inline GetRefData(int* data, int type) : data(data), type(type) {}

	template<typename T>
	inline T& as()
	{
		return *(T*)data;
	}
};

GetRefData GetRef(Var& v)
{
	assert(v.type == V_REF);
	switch(v.ref_type)
	{
	case REF_LOCAL:
		{
			assert(v.ref_index < local.size());
			Var& vr = local[v.ref_index];
			return GetRefData(&vr.value, vr.type);
		}
	case REF_GLOBAL:
		{
			assert(v.ref_index < global.size());
			Var& vr = global[v.ref_index];
			return GetRefData(&vr.value, vr.type);
		}
	case REF_MEMBER:
		{
			Class* c = v.ref_class;
			Member* m = c->type->members[v.ref_index];
			return GetRefData((int*)c->at_data(m->offset), m->type);
		}
	case REF_CODE:
		return GetRefData(v.ref_adr, v.ref_var_type);
	default:
		assert(0);
		return GetRefData(nullptr, 0);
	}
}

void ExecuteFunction(Function& f)
{
	assert(f.arg_infos.size() < 15u);
	int packedArgs[16];
	int packed = 0;
	void* retptr = nullptr;
	bool in_mem = false;

	Type* result_type = run_module->GetType(f.result.core);
	if(f.result.core == V_STRING)
	{
		// string return value
		Str* str = CreateStr();
		packedArgs[packed++] = (int)(&str->s);
		retptr = str;
	}
	else if(result_type->IsClass())
	{
		// class return value
		Type* type = run_module->GetType(f.result.core);
		Class* c = Class::Create(type);
		retptr = c;
		if(type->size > 8 || IS_SET(type->flags, Type::Complex))
		{
			packedArgs[packed++] = (int)c->data();
			in_mem = true;
		}
	}

	// verify and pack args
	assert(stack.size() >= f.arg_infos.size());
	for(uint i = 0; i < f.arg_infos.size(); ++i)
	{
		Var& v = stack.at(stack.size() - f.arg_infos.size() + i);
		ArgInfo& arg = f.arg_infos[i];
		int value;
		if(arg.type.special == SV_NORMAL)
		{
			assert(v.type == arg.type.core);
			switch(v.type)
			{
			case V_BOOL:
			case V_CHAR:
			case V_INT:
			case V_FLOAT:
				value = v.value;
				break;
			case V_STRING:
				value = (int)&v.str->s;
				break;
			default:
				assert(run_module->GetType(v.type)->IsClass());
				value = (int)v.clas->data();
				break;
			}
		}
		else
		{
			assert(arg.type.special == SV_REF);
			assert(v.type == V_REF);
			GetRefData refdata = GetRef(v);
			assert(refdata.type == arg.type.core);
			value = (int)refdata.data;
		}
		
		packedArgs[packed++] = value;
	}

	// set this
	void* _this;
	int* args;
	uint packedSize;
	uint espRestore;
	if(!IS_SET(f.flags, CommonFunction::F_THISCALL))
	{
		_this = nullptr;
		args = &packedArgs[0];
		packedSize = packed * 4;
		espRestore = packedSize;
	}
	else
	{
		_this = (void*)packedArgs[0];
		args = &packedArgs[1];
		packedSize = packed * 4 - 4;
		espRestore = 0;
	}

	// call
	void* clbk = f.clbk;
	union
	{
		struct
		{
			int low;
			int high;
		};
		__int64 qw;
	} result;
	float fresult;	
	
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
		mov ecx, _this;

		// call
		call clbk;

		// get result
		add esp, espRestore;
		lea ecx, result;
		mov [ecx], eax;
		mov 4[ecx], edx;
		fstp fresult;
		pop ecx;
	};

	// update stack
	for(uint i = 0; i < f.arg_infos.size(); ++i)
	{
		Var& v = stack.back();
		ReleaseRef(v);
		stack.pop_back();
	}

	// push result
	if(f.result.special == SV_NORMAL)
	{
		switch(f.result.core)
		{
		case V_VOID:
			break;
		case V_BOOL:
			stack.push_back(Var(result.low != 0));
			break;
		case V_CHAR:
			stack.push_back(Var((char)result.low));
			break;
		case V_INT:
			stack.push_back(Var(result.low));
			break;
		case V_FLOAT:
			stack.push_back(Var(fresult));
			break;
		case V_STRING:
			stack.push_back(Var((Str*)retptr));
			break;
		default:
			{
				assert(result_type->IsClass());
				Class* c = (Class*)retptr;
				if(!in_mem)
					memcpy(c->data(), &result, c->type->size);
				stack.push_back(Var(c));
			}
			break;
		}
	}
	else
	{
		assert(f.result.core == V_BOOL || f.result.core == V_CHAR || f.result.core == V_INT || f.result.core == V_FLOAT);
		stack.push_back(Var(V_REF, REF_CODE, result.low, f.result.core));
	}
}

bool CompareVar(Var& v, const VarType& type)
{
	if(type.special == SV_NORMAL)
		return (v.type == type.core);
	else if(v.type != V_REF)
		return false;
	else
	{
		GetRefData data = GetRef(v);
		return (data.type == type.core);
	}
}

void MakeSingleInstance(Var& v)
{
	Type* type = run_module->GetType(v.type);
	assert(type->IsStruct());
	assert(v.clas->refs >= START_REF_COUNT);
	if(v.clas->refs <= START_REF_COUNT)
		return;
	Class* copy = Class::Copy(v.clas);
	ReleaseRef(v);
	v.clas = copy;
}

void SetFromStack(Var& v)
{
	assert(!stack.empty());
	Var& s = stack.back();
	assert(v.type == V_VOID || v.type == s.type);
	Type* type = run_module->GetType(v.type);
	if(!type->IsStruct())
	{
		// free what was in variable previously
		ReleaseRef(v);
		// incrase reference for new var
		AddRef(s);
		v = s;
	}
	else
		memcpy(v.clas->data(), s.clas->data(), type->size);
}

void Cast(Var& v, int type)
{
	assert(v.type == V_BOOL || v.type == V_CHAR || v.type == V_INT || v.type == V_FLOAT || v.type == V_STRING);
	assert(type == V_BOOL || type == V_CHAR || type == V_INT || type == V_FLOAT || type == V_STRING);
	assert(v.type != type);

#define COMBINE(x,y) ((x & 0xFF) | ((y & 0xFF) << 8))

	switch(COMBINE(v.type, type))
	{
	case COMBINE(V_BOOL,V_CHAR):
		v.cvalue = (v.bvalue ? 't' : 'f');
		v.type = V_CHAR;
		break;
	case COMBINE(V_BOOL,V_INT):
		v.value = (v.bvalue ? 1 : 0);
		v.type = V_INT;
		break;
	case COMBINE(V_BOOL,V_FLOAT):
		v.fvalue = (v.bvalue ? 1.f : 0.f);
		v.type = V_FLOAT;
		break;
	case COMBINE(V_BOOL,V_STRING):
		v.str = CreateStr(v.bvalue ? "true" : "false");
		v.type = V_STRING;
		break;
	case COMBINE(V_CHAR,V_BOOL):
		v.bvalue = (v.cvalue != 0);
		v.type = V_BOOL;
		break;
	case COMBINE(V_CHAR,V_INT):
		v.value = (int)v.cvalue;
		v.type = V_INT;
		break;
	case COMBINE(V_CHAR,V_FLOAT):
		v.fvalue = (float)v.cvalue;
		v.type = V_FLOAT;
		break;
	case COMBINE(V_CHAR,V_STRING):
		v.str = CreateStr(Format("%c", v.cvalue));
		v.type = V_STRING;
		break;
	case COMBINE(V_INT,V_BOOL):
		v.bvalue = (v.value != 0);
		v.type = V_BOOL;
		break;
	case COMBINE(V_INT,V_CHAR):
		v.cvalue = (char)v.value;
		v.type = V_CHAR;
		break;
	case COMBINE(V_INT,V_FLOAT):
		v.fvalue = (float)v.value;
		v.type = V_FLOAT;
		break;
	case COMBINE(V_INT,V_STRING):
		v.str = CreateStr(Format("%d", v.value));
		v.type = V_STRING;
		break;
	case COMBINE(V_FLOAT,V_BOOL):
		v.bvalue = (v.fvalue != 0.f);
		v.type = V_BOOL;
		break;
	case COMBINE(V_FLOAT,V_CHAR):
		v.cvalue = (char)v.fvalue;
		v.type = V_CHAR;
		break;
	case COMBINE(V_FLOAT,V_INT):
		v.value = (int)v.fvalue;
		v.type = V_INT;
		break;
	case COMBINE(V_FLOAT,V_STRING):
		v.str = CreateStr(Format("%g", v.fvalue));
		v.type = V_STRING;
		break;
	default:
		assert(0);
		break;
	}

#undef COMBINE
}

void RunInternal(ReturnValue& retval)
{
	int* start = run_module->code.data();
	int* end = start + run_module->code.size();
	int* c = start + run_module->entry_point;

	while(true)
	{
		Op op = (Op)*c++;
		switch(op)
		{
		case PUSH:
			{
				assert(!stack.empty());
				Var& v = stack.back();
				AddRef(v);
				stack.push_back(v);
			}
			break;
		case PUSH_TRUE:
			stack.push_back(Var(true));
			break;
		case PUSH_FALSE:
			stack.push_back(Var(false));
			break;
		case PUSH_CHAR:
			{
				char val = *(char*)c;
				++c;
				stack.push_back(Var(val));
			}
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
				assert(str_index < run_module->strs.size());
				Str* str = run_module->strs[str_index];
				str->refs++;
				stack.push_back(Var(str));
			}
			break;
		case PUSH_LOCAL:
			{
				uint local_index = *c++;
				assert(current_function != -1 && (uint)current_function < run_module->ufuncs.size());
				assert(run_module->ufuncs[current_function].locals > local_index);
				Var& v = local[locals_offset + local_index];
				AddRef(v);
				stack.push_back(v);
			}
			break;
		case PUSH_LOCAL_REF:
			{
				uint local_index = *c++;
				assert(current_function != -1 && (uint)current_function < run_module->ufuncs.size());
				assert(run_module->ufuncs[current_function].locals > local_index);
				uint index = locals_offset + local_index;
				Var& v = local[index];
				assert(v.type != V_VOID && v.type != V_REF && v.type != V_STRING && !run_module->GetType(v.type)->IsClass());
				stack.push_back(Var(REF_LOCAL, index, nullptr));
			}
			break;
		case PUSH_GLOBAL:
			{
				uint global_index = *c++;
				assert(global_index < global.size());
				Var& v = global[global_index];
				AddRef(v);
				stack.push_back(v);
			}
			break;
		case PUSH_GLOBAL_REF:
			{
				uint global_index = *c++;
				assert(global_index < global.size());
				Var& v = global[global_index];
				assert(v.type != V_VOID && v.type != V_REF && v.type != V_STRING && !run_module->GetType(v.type)->IsClass());
				stack.push_back(Var(REF_GLOBAL, global_index, nullptr));
			}
			break;
		case PUSH_ARG:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < run_module->ufuncs.size());
				assert(run_module->ufuncs[current_function].args.size() > arg_index);
				Var& v = local[args_offset + arg_index];
				AddRef(v);
				stack.push_back(v);
			}
			break;
		case PUSH_ARG_REF:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < run_module->ufuncs.size());
				assert(run_module->ufuncs[current_function].args.size() > arg_index);
				uint index = args_offset + arg_index;
				Var& v = local[index];
				assert(v.type != V_VOID && v.type != V_REF && v.type != V_STRING && !run_module->GetType(v.type)->IsClass());
				stack.push_back(Var(REF_LOCAL, index, nullptr));
			}
			break;
		case PUSH_MEMBER:
			{
				assert(!stack.empty());
				Var& v = stack.back();
				assert(run_module->GetType(v.type)->IsClass());
				Type* type = run_module->GetType(v.type);
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Member* m = type->members[member_index];
				Class* c = v.clas;
				stack.pop_back();
				switch(m->type)
				{
				case V_BOOL:
					stack.push_back(Var(c->at<bool>(m->offset)));
					break;
				case V_CHAR:
					stack.push_back(Var(c->at<char>(m->offset)));
					break;
				case V_INT:
					stack.push_back(Var(c->at<int>(m->offset)));
					break;
				case V_FLOAT:
					stack.push_back(Var(c->at<float>(m->offset)));
					break;
				default:
					assert(0);
					break;
				}
				c->Release();
			}
			break;
		case PUSH_MEMBER_REF:
			{
				// don't release class ref because MEMBER_REF increase by 1
				assert(!stack.empty());
				Var& v = stack.back();
				assert(run_module->GetType(v.type)->IsClass());
				Type* type = run_module->GetType(v.type);
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Member* m = type->members[member_index];
				assert(m->type == V_BOOL || m->type == V_CHAR || m->type == V_INT || m->type == V_FLOAT);
				Class* c = v.clas;
				stack.pop_back();
				stack.push_back(Var(REF_MEMBER, member_index, c));
			}
			break;
		case PUSH_THIS_MEMBER:
			{
				// check is inside script class function
				assert(current_function != -1);
				assert((uint)current_function < run_module->ufuncs.size());
				UserFunction& f = run_module->ufuncs[current_function];
				assert(run_module->GetType(f.type)->IsClass());
				Type* type = run_module->GetType(f.type);
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Var& v = local[args_offset];
				assert(v.type == f.type);
				Class* c = v.clas;
				Member* m = type->members[member_index];

				// push value
				switch(m->type)
				{
				case V_BOOL:
					stack.push_back(Var(c->at<bool>(m->offset)));
					break;
				case V_CHAR:
					stack.push_back(Var(c->at<char>(m->offset)));
					break;
				case V_INT:
					stack.push_back(Var(c->at<int>(m->offset)));
					break;
				case V_FLOAT:
					stack.push_back(Var(c->at<float>(m->offset)));
					break;
				default:
					assert(0);
					break;
				}
			}
			break;
		case PUSH_THIS_MEMBER_REF:
			{
				// check is inside script class function
				assert(current_function != -1);
				assert((uint)current_function < run_module->ufuncs.size());
				UserFunction& f = run_module->ufuncs[current_function];
				assert(run_module->GetType(f.type)->IsClass());
				Type* type = run_module->GetType(f.type);
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Var& v = local[args_offset];
				assert(v.type == f.type);
				Class* c = v.clas;
				++c->refs;
				Member* m = type->members[member_index];

				// push reference
				assert(m->type == V_BOOL || m->type == V_CHAR || m->type == V_INT || m->type == V_FLOAT);
				stack.push_back(Var(REF_MEMBER, member_index, c));
			}
			break;
		case PUSH_TMP:
			stack.push_back(tmpv);
			break;
		case PUSH_INDEX:
			{
				assert(stack.size() >= 2u);
				Var vindex = stack.back();
				assert(vindex.type == V_INT);
				uint index = vindex.value;
				stack.pop_back();
				Var& v = stack.back();
				assert(v.type == V_STRING);
				assert(index < v.str->s.length());
				char c = v.str->s[index];
				ReleaseRef(v);
				v.type = V_CHAR;
				v.cvalue = c;
			}
			break;
		case POP:
			{
				assert(!stack.empty());
				Var& v = stack.back();
				ReleaseRef(v);
				stack.pop_back();
			}
			break;
		case SET_LOCAL:
			{
				uint local_index = *c++;
				assert(current_function != -1 && (uint)current_function < run_module->ufuncs.size());
				assert(run_module->ufuncs[current_function].locals > local_index);
				Var& v = local[locals_offset + local_index];
				SetFromStack(v);
			}
			break;
		case SET_GLOBAL:
			{
				uint global_index = *c++;
				assert(global_index < global.size());
				Var& v = global[global_index];
				SetFromStack(v);
			}
			break;
		case SET_ARG:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < run_module->ufuncs.size());
				assert(run_module->ufuncs[current_function].args.size() > arg_index);
				Var& v = local[args_offset + arg_index];
				SetFromStack(v);
			}
			break;
		case SET_MEMBER:
			{
				// get value
				assert(stack.size() >= 2u);
				Var v = stack.back();
				stack.pop_back();

				// get class
				Var& cv = stack.back();
				assert(run_module->GetType(cv.type)->IsClass());
				Type* type = run_module->GetType(cv.type);
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Member* m = type->members[member_index];
				assert(v.type == m->type);
				Class* c = cv.clas;				

				switch(m->type)
				{
				case V_BOOL:
					c->at<bool>(m->offset) = v.bvalue;
					break;
				case V_CHAR:
					c->at<char>(m->offset) = v.cvalue;
					break;
				case V_INT:
					c->at<int>(m->offset) = v.value;
					break;
				case V_FLOAT:
					c->at<float>(m->offset) = v.fvalue;
					break;
				default:
					assert(0);
					break;
				}

				c->Release();
				stack.back() = v;
			}
			break;
		case SET_THIS_MEMBER:
			{
				assert(!stack.empty());
				Var& v = stack.back();

				// check is inside script class function
				assert(current_function != -1);
				assert((uint)current_function < run_module->ufuncs.size());
				UserFunction& f = run_module->ufuncs[current_function];
				assert(run_module->GetType(f.type)->IsClass());
				Type* type = run_module->GetType(f.type);
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Var& vl = local[args_offset];
				assert(vl.type == f.type);
				Class* c = vl.clas;
				Member* m = type->members[member_index];
				
				switch(m->type)
				{
				case V_BOOL:
					c->at<bool>(m->offset) = v.bvalue;
					break;
				case V_CHAR:
					c->at<char>(m->offset) = v.cvalue;
					break;
				case V_INT:
					c->at<int>(m->offset) = v.value;
					break;
				case V_FLOAT:
					c->at<float>(m->offset) = v.fvalue;
					break;
				default:
					assert(0);
					break;
				}
			}
			break;
		case SET_TMP:
			assert(!stack.empty());
			tmpv = stack.back();
			break;
		case SET_INDEX:
			{
				assert(stack.size() >= 3u);
				Var x = stack.back();
				stack.pop_back();
				assert(x.type == V_CHAR);
				Var vindex = stack.back();
				stack.pop_back();
				assert(vindex.type == V_INT);
				uint index = vindex.value;
				Var& arr = stack.back();
				assert(arr.type == V_STRING);
				assert(index <= arr.str->s.length());
				arr.str->s[index] = x.cvalue;
				ReleaseRef(arr);
				arr = x;
			}
			break;
		case SWAP:
			{
				uint index = *c++;
				assert(index + 1 < stack.size());
				auto a = stack.rbegin() + index;
				auto b = a + 1;
				std::iter_swap(a, b);
			}
			break;
		case CAST:
			{
				int type = *c++;
				assert(!stack.empty());
				Var& v = stack.back();
				Cast(v, type);
			}
			break;
		case NEG:
		case NOT:
		case BIT_NOT:
		case INC:
		case DEC:
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
					auto data = GetRef(v);
					ReleaseRef(v);
					v.type = data.type;
					v.value = *data.data;
					AddRef(v);
				}
				else
				{
					assert(v.type == V_CHAR || v.type == V_INT || v.type == V_FLOAT);
					if(op == INC)
					{
						switch(v.type)
						{
						case V_CHAR:
							v.cvalue++;
							break;
						case V_INT:
							v.value++;
							break;
						case V_FLOAT:
							v.fvalue++;
							break;
						}
					}
					else
					{
						switch(v.type)
						{
						case V_CHAR:
							v.cvalue--;
							break;
						case V_INT:
							v.value--;
							break;
						case V_FLOAT:
							v.fvalue--;
							break;
						}
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
		case IS:
		case SET_ADR:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				if(op != SET_ADR)
					assert(left.type == right.type);
				if(op == ADD)
					assert(left.type == V_INT || left.type == V_FLOAT || left.type == V_STRING);
				else if(op == EQ || op == NOT_EQ)
					assert(left.type == V_BOOL || left.type == V_CHAR || left.type == V_INT || left.type == V_FLOAT || left.type == V_STRING);
				else if(op == AND || op == OR)
					assert(left.type == V_BOOL);
				else if(op == BIT_AND || op == BIT_OR || op == BIT_XOR || op == BIT_LSHIFT || op == BIT_RSHIFT)
					assert(left.type == V_INT);
				else if(op == IS)
					assert(left.type == V_STRING || run_module->GetType(left.type)->IsClass() || left.type == V_REF);
				else if(op == SET_ADR)
				{
					assert(left.type == V_REF);
					assert(!run_module->GetType(right.type)->IsRef());
				}
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
						left.str = CreateStr(result.c_str());
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
					switch(left.type)
					{
					case V_BOOL:
						left.bvalue = (left.bvalue == right.bvalue);
						break;
					case V_CHAR:
						left.bvalue = (left.cvalue == right.cvalue);
						break;
					case V_INT:
						left.bvalue = (left.value == right.value);
						break;
					case V_FLOAT:
						left.bvalue = (left.fvalue == right.fvalue);
						break;
					case V_STRING:
						left.bvalue = (left.str->s == right.str->s);
						break;
					}
					left.type = V_BOOL;
					break;
				case NOT_EQ:
					switch(left.type)
					{
					case V_BOOL:
						left.bvalue = (left.bvalue != right.bvalue);
						break;
					case V_CHAR:
						left.bvalue = (left.cvalue != right.cvalue);
						break;
					case V_INT:
						left.bvalue = (left.value != right.value);
						break;
					case V_FLOAT:
						left.bvalue = (left.fvalue != right.fvalue);
						break;
					case V_STRING:
						left.bvalue = (left.str->s != right.str->s);
						break;
					}
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
				case IS:
					{
						bool result;
						if(left.type == V_STRING)
							result = (left.str == right.str);
						else if(left.type == V_REF)
						{
							assert(left.ref_type == right.ref_type);
							GetRefData refl = GetRef(left);
							GetRefData refr = GetRef(right);
							result = (refl.data == refr.data);
						}
						else
							result = (left.clas == right.clas);
						ReleaseRef(right);
						ReleaseRef(left);
						left.type = V_BOOL;
						left.bvalue = result;
					}
					break;
				case SET_ADR:
					{
						GetRefData ref = GetRef(left);
						ReleaseRef(left);
						assert(ref.type == right.type);
						memcpy(ref.data, &right.value, run_module->GetType(right.type)->size);
						stack.pop_back();
						stack.push_back(right);
					}
					break;
				}
			}
			break;
		case RET:
			if(current_function == -1)
			{
				// set & validate return value
				assert(local.empty());
				if(run_module->result == V_VOID)
				{
					assert(stack.empty());
					retval.type = cas::ReturnValue::Void;
				}
				else
				{
					assert(stack.size() == 1u);
					Var& v = stack.back();
					assert(v.type == run_module->result);
					retval.type = (cas::ReturnValue::Type)run_module->result;
					retval.int_value = v.value;
					stack.pop_back();
				}
				return;
			}
			else
			{
				assert((uint)current_function < run_module->ufuncs.size());
				UserFunction& f = run_module->ufuncs[current_function];
				uint to_pop = f.locals + f.args.size();
				assert(local.size() > to_pop);
				Var& func_mark = *(local.end() - to_pop - 1);
				assert(func_mark.type == V_SPECIAL && (func_mark.special_type == V_FUNCTION || func_mark.special_type == V_CTOR));
				bool is_ctor = (func_mark.special_type == V_CTOR);
				if(is_ctor)
					--to_pop;
				while(to_pop--)
				{
					Var& v = local.back();
					ReleaseRef(v);
					local.pop_back();
				}
				Class* thi = nullptr;
				if(is_ctor)
				{
					assert(local.back().type == f.type);
					thi = local.back().clas;
					local.pop_back();
				}
				c = start + func_mark.value2;
				current_function = func_mark.value1;
				local.pop_back();
				if(current_function != -1)
				{
					// checking local stack
					assert((uint)current_function < run_module->ufuncs.size());
					UserFunction& f = run_module->ufuncs[current_function];
					uint count = 1 + f.locals + f.args.size();
					assert(local.size() >= count);
					Var& d = *(local.end() - count);
					assert(d.type == V_SPECIAL && (d.special_type == V_FUNCTION || d.special_type == V_CTOR));
					locals_offset = local.size() - f.locals;
					args_offset = locals_offset - f.args.size();
				}
				if(thi)
					stack.push_back(Var(thi));
				assert(expected_stack.back() == stack.size());
				if(f.result.core != V_VOID)
					assert(CompareVar(stack.back(), f.result));
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
				Function* f = run_module->GetFunction(f_idx);
				ExecuteFunction(*f);
			}
			break;
		case CALLU:
			{
				uint f_idx = *c++;
				assert(f_idx < run_module->ufuncs.size());
				UserFunction& f = run_module->ufuncs[f_idx];
				// mark function call
				uint pos = c - start;
				local.push_back(Var(V_SPECIAL, V_FUNCTION, current_function, pos));
				// handle args
				assert(stack.size() >= f.args.size());
				args_offset = local.size();
				local.resize(local.size() + f.args.size());
				for(uint i = 0, count = f.args.size(); i < count; ++i)
				{
					assert(CompareVar(stack.back(), f.args[count - 1 - i]));
					local[args_offset + count - 1 - i] = stack.back();
					stack.pop_back();
				}
				// handle locals
				locals_offset = local.size();
				local.resize(local.size() + f.locals);
				// call
				uint expected = stack.size();
				if(f.result.core != V_VOID)
					++expected;
				expected_stack.push_back(expected);
				current_function = f_idx;
				c = start + f.pos;
			}
			break;
		case CALLU_CTOR:
			{
				uint f_idx = *c++;
				assert(f_idx < run_module->ufuncs.size());
				UserFunction& f = run_module->ufuncs[f_idx];
				// mark function call
				uint pos = c - start;
				local.push_back(Var(V_SPECIAL, V_CTOR, current_function, pos));
				// push this
				args_offset = local.size();
				assert(run_module->GetType(f.type)->IsClass());
				local.resize(local.size() + f.args.size());
				local[args_offset] = Var(Class::Create(run_module->GetType(f.type)));
				// handle args
				assert(stack.size() >= f.args.size() - 1);
				for(uint i = 1, count = f.args.size(); i < count; ++i)
				{
					assert(CompareVar(stack.back(), f.args[count - i]));
					local[args_offset + count - i] = stack.back();
					stack.pop_back();
				}
				// handle locals
				locals_offset = local.size();
				local.resize(local.size() + f.locals);
				// call
				uint expected = stack.size() + 1;
				expected_stack.push_back(expected);
				current_function = f_idx;
				c = start + f.pos;
			}
			break;
		case CTOR:
			{
				uint type_index = *c++;
				assert(run_module->GetType(type_index)->IsClass());
				Type* type = run_module->GetType(type_index);
				Class* c = Class::Create(type);
				stack.push_back(Var(c));
			}
			break;
		case COPY:
			{
				assert(!stack.empty());
				Var& v = stack.back();
				MakeSingleInstance(v);
			}
			break;
		case COPY_ARG:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < run_module->ufuncs.size());
				assert(run_module->ufuncs[current_function].args.size() > arg_index);
				Var& v = local[args_offset + arg_index];
				MakeSingleInstance(v);
			}
			break;
		default:
			assert(0);
			break;
		}
		assert(c < end);
	}
}

void Run(RunModule& _run_module, ReturnValue& _retval)
{
	run_module = &_run_module;

	// prepare stack
	tmpv = Var();
	stack.clear();
	global.clear();
	global.resize(run_module->globals);
	local.clear();
	current_function = -1;
#ifdef CHECK_LEAKS
	all_clases.clear();
#endif

	RunInternal(_retval);

	// cleanup
	for(Var& v : global)
		ReleaseRef(v);
#ifdef CHECK_LEAKS
	for(Class* c : all_clases)
	{
		assert(c->refs == 1);
		delete c;
	}
#endif
}
