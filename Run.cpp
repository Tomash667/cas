#include "Pch.h"
#include "Base.h"
#include "Run.h"
#include "Function.h"
#include "Op.h"
#include "Cas.h"

enum REF_TYPE
{
	REF_GLOBAL,
	REF_LOCAL,
	REF_MEMBER
};

enum SPECIAL_VAR
{
	V_FUNCTION,
	V_CTOR
};

//#define CHECK_LEAKS

#ifdef CHECK_LEAKS
struct Class;
vector<Class*> all_clases;
#endif

struct Class
{
	int refs;
	Type* type;

	inline int* data()
	{
		return ((int*)this) + 2;
	}
	
	inline byte* at_data(uint offset)
	{
		return ((byte*)data()) + offset;
	}

	template<typename T>
	inline T& at(uint offset)
	{
		return *(T*)at_data(offset);
	}

	inline static Class* Create(Type* type)
	{
		assert(type);
		byte* data = new byte[type->size + 8];
		memset(data + 8, 0, type->size);
		Class* c = (Class*)data;
		c->refs = 1;
		c->type = type;
#ifdef CHECK_LEAKS
		++c->refs;
		all_clases.push_back(c);
#endif
		return c;
	}

	inline void Release()
	{
		if(--refs == 0)
			delete this;
	}
};

struct Var
{
	int type;
	union
	{
		bool bvalue;
		int value;
		float fvalue;
		Str* str;
		struct
		{
			REF_TYPE ref_type;
			Class* ref_class;
			uint ref_index;
		};
		Class* clas;
		struct
		{
			int special_type;
			int value1;
			int value2;
		};
	};

	inline explicit Var() : type(V_VOID) {}
	inline explicit Var(bool bvalue) : type(V_BOOL), bvalue(bvalue) {}
	inline explicit Var(int value) : type(V_INT), value(value) {}
	inline explicit Var(float fvalue) : type(V_FLOAT), fvalue(fvalue) {}
	inline explicit Var(Str* str) : type(V_STRING), str(str) {}
	inline Var(REF_TYPE ref_type, uint ref_index, Class* ref_class) : type(V_REF), ref_type(ref_type), ref_index(ref_index), ref_class(ref_class) {}
	inline explicit Var(Class* clas) : type(clas->type->index), clas(clas) {}
	inline Var(int type, int special_type, int value1, int value2) : type(type), special_type(special_type), value1(value1), value2(value2) {}
};

vector<Var> stack, global, local;
vector<uint> expected_stack;
int current_function, args_offset, locals_offset;
extern cas::ReturnValue return_value;

void AddRef(Var& v)
{
	assert(v.type != V_VOID);
	if(v.type == V_STRING)
		v.str->refs++;
	else if(v.type >= V_CLASS)
		v.clas->refs++;
}

void ReleaseRef(Var& v)
{
	if(v.type == V_STRING)
		v.str->Release();
	else if(v.type >= V_CLASS)
		v.clas->Release();
	else if(v.type == V_REF && v.ref_type == REF_MEMBER)
		v.ref_class->Release();
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
	if(v.ref_type == REF_LOCAL)
	{
		assert(v.ref_index < local.size());
		Var& vr = local[v.ref_index];
		return GetRefData(&vr.value, vr.type);
	}
	else if(v.ref_type == REF_GLOBAL)
	{
		assert(v.ref_index < global.size());
		Var& vr = global[v.ref_index];
		return GetRefData(&vr.value, vr.type);
	}
	else
	{
		assert(v.ref_type == REF_MEMBER);
		Class* c = v.ref_class;
		Member* m = c->type->members[v.ref_index];
		return GetRefData((int*)c->at_data(m->offset), m->type);
	}
}

void ExecuteFunction(Function& f)
{
	assert(f.arg_infos.size() < 15u);
	int packedArgs[16];
	int packed = 0;
	void* retptr = nullptr;
	bool in_mem = false;

	assert(f.result.special == SV_NORMAL);
	if(f.result.core == V_STRING)
	{
		// string return value
		Str* str = Str::Get();
		str->refs = 1;
		packedArgs[packed++] = (int)(&str->s);
		retptr = str;
	}
	else if(f.result.core >= V_CLASS)
	{
		// class return value
		Type* type = types[f.result.core];
		Class* c = Class::Create(type);
		retptr = c;
		if(type->size > 8 || !type->pod)
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
			case V_INT:
			case V_FLOAT:
				value = v.value;
				break;
			case V_STRING:
				value = (int)&v.str->s;
				break;
			default:
				if(v.type >= V_CLASS)
					value = (int)v.clas->data();
				else
					assert(0);
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
	switch(f.result.core)
	{
	case V_VOID:
		break;
	case V_BOOL:
		stack.push_back(Var(result.low != 0));
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
		if(f.result.core >= V_CLASS)
		{
			Class* c = (Class*)retptr;
			if(!in_mem)
				memcpy(c->data(), &result, c->type->size);
			stack.push_back(Var(c));
		}
		else
			assert(0);
		break;
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

void RunCode(RunContext& ctx)
{
	stack.clear();
	global.clear();
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
				AddRef(v);
				stack.push_back(v);
			}
			break;
		case PUSH_LOCAL_REF:
			{
				uint local_index = *c++;
				assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
				assert(ctx.ufuncs[current_function].locals > local_index);
				uint index = locals_offset + local_index;
				Var& v = local[index];
				assert(v.type != V_VOID && v.type != V_REF && v.type != V_STRING && v.type < V_CLASS);
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
				assert(v.type != V_VOID && v.type != V_REF && v.type != V_STRING && v.type < V_CLASS);
				stack.push_back(Var(REF_GLOBAL, global_index, nullptr));
			}
			break;
		case PUSH_ARG:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
				assert(ctx.ufuncs[current_function].args.size() > arg_index);
				Var& v = local[args_offset + arg_index];
				AddRef(v);
				stack.push_back(v);
			}
			break;
		case PUSH_ARG_REF:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
				assert(ctx.ufuncs[current_function].args.size() > arg_index);
				uint index = args_offset + arg_index;
				Var& v = local[index];
				assert(v.type != V_VOID && v.type != V_REF && v.type != V_STRING && v.type < V_CLASS);
				stack.push_back(Var(REF_LOCAL, index, nullptr));
			}
			break;
		case PUSH_MEMBER:
			{
				assert(!stack.empty());
				Var& v = stack.back();
				assert(v.type >= V_CLASS);
				Type* type = types[v.type];
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
				assert(v.type >= V_CLASS);
				Type* type = types[v.type];
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Member* m = type->members[member_index];
				assert(m->type == V_BOOL || m->type == V_INT || m->type == V_FLOAT);
				Class* c = v.clas;
				stack.pop_back();
				stack.push_back(Var(REF_MEMBER, member_index, c));
			}
			break;
		case PUSH_THIS_MEMBER:
			{
				// check is inside script class function
				assert(current_function != -1);
				assert((uint)current_function < ctx.ufuncs.size());
				UserFunction& f = ctx.ufuncs[current_function];
				assert(f.type >= V_CLASS && (uint)f.type < types.size());
				Type* type = types[f.type];
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
				assert((uint)current_function < ctx.ufuncs.size());
				UserFunction& f = ctx.ufuncs[current_function];
				assert(f.type >= V_CLASS && (uint)f.type < types.size());
				Type* type = types[f.type];
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Var& v = local[args_offset];
				assert(v.type == f.type);
				Class* c = v.clas;
				++c->refs;
				Member* m = type->members[member_index];

				// push reference
				assert(m->type == V_BOOL || m->type == V_INT || m->type == V_FLOAT);
				stack.push_back(Var(REF_MEMBER, member_index, c));
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
				assert(current_function != -1 && (uint)current_function < ctx.ufuncs.size());
				assert(ctx.ufuncs[current_function].locals > local_index);
				assert(!stack.empty());
				Var& v = local[locals_offset + local_index];
				assert(v.type == V_VOID || v.type == stack.back().type);
				// free what was in variable previously
				ReleaseRef(v);
				// incrase reference for new var
				Var& s = stack.back();
				AddRef(s);
				v = s;
			}
			break;
		case SET_GLOBAL:
			{
				uint global_index = *c++;
				assert(global_index < global.size());
				assert(!stack.empty());
				Var& v = global[global_index];
				assert(v.type == V_VOID || v.type == stack.back().type);
				// free what was in variable previously
				ReleaseRef(v);
				Var& s = stack.back();
				// incrase reference for new var
				AddRef(s);
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
				assert(v.type == stack.back().type);
				// free what was in variable previously
				ReleaseRef(v);
				Var& s = stack.back();
				// incrase reference for new var
				AddRef(s);
				v = s;
			}
			break;
		case SET_MEMBER:
			{
				// get class
				assert(stack.size() >= 2u);
				Var& cv = stack.back();
				assert(cv.type >= V_CLASS);
				Type* type = types[cv.type];
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Member* m = type->members[member_index];
				Class* c = cv.clas;
				stack.pop_back();

				// get value
				Var& v = stack.back();
				assert(v.type == m->type);

				switch(m->type)
				{
				case V_BOOL:
					c->at<bool>(m->offset) = v.bvalue;
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
			}
			break;
		case SET_THIS_MEMBER:
			{
				assert(!stack.empty());
				Var& v = stack.back();

				// check is inside script class function
				assert(current_function != -1);
				assert((uint)current_function < ctx.ufuncs.size());
				UserFunction& f = ctx.ufuncs[current_function];
				assert(f.type >= V_CLASS && (uint)f.type < types.size());
				Type* type = types[f.type];
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
		case CAST:
			// allowed casts bool/int/float -> anything
			{
				int type = *c++;
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
					v.type = data.type;
					v.value = *data.data;
					AddRef(v);
				}
				else
				{
					assert(v.type == V_INT || v.type == V_FLOAT);
					if(op == INC)
					{
						if(v.type == V_INT)
							v.value++;
						else
							v.fvalue++;
					}
					else
					{
						if(v.type == V_INT)
							v.value--;
						else
							v.fvalue--;
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
					assert(left.type == V_BOOL || left.type == V_INT || left.type == V_FLOAT);
				else if(op == AND || op == OR)
					assert(left.type == V_BOOL);
				else if(op == BIT_AND || op == BIT_OR || op == BIT_XOR || op == BIT_LSHIFT || op == BIT_RSHIFT)
					assert(left.type == V_INT);
				else if(op == IS)
					assert(left.type == V_STRING || left.type >= V_CLASS);
				else if(op == SET_ADR)
				{
					assert(left.type == V_REF);
					assert(!types[right.type]->is_ref);
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
				case IS:
					if(left.type == V_STRING)
						left.bvalue = (left.str == right.str);
					else
						left.bvalue = (left.clas == right.clas);
					left.type = V_BOOL;
					break;
				case SET_ADR:
					{
						GetRefData ref = GetRef(left);
						assert(ref.type == right.type);
						memcpy(ref.data, &right.value, types[right.type]->size);
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
				assert(local.empty());
				if(ctx.result == V_VOID)
				{
					assert(stack.empty());
					return_value.type = cas::ReturnValue::Void;
				}
				else
				{
					assert(stack.size() == 1u);
					Var& v = stack.back();
					assert(v.type == ctx.result);
					return_value.type = (cas::ReturnValue::Type)ctx.result;
					return_value.int_value = v.value;
					stack.pop_back();
				}
#ifdef CHECK_LEAKS
				for(Class* c : all_clases)
					assert(c->refs == 1);
#endif
				return;
			}
			else
			{
				assert((uint)current_function < ctx.ufuncs.size());
				UserFunction& f = ctx.ufuncs[current_function];
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
					assert((uint)current_function < ctx.ufuncs.size());
					UserFunction& f = ctx.ufuncs[current_function];
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
				assert(f_idx < ctx.ufuncs.size());
				UserFunction& f = ctx.ufuncs[f_idx];
				// mark function call
				uint pos = c - start;
				local.push_back(Var(V_SPECIAL, V_CTOR, current_function, pos));
				// push this
				args_offset = local.size();
				assert(f.type >= V_CLASS && (uint)f.type < types.size());
				local.resize(local.size() + f.args.size());
				local[args_offset] = Var(Class::Create(types[f.type]));
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
				assert(type_index >= V_CLASS && type_index < types.size());
				Type* type = types[type_index];
				Class* c = Class::Create(type);
				stack.push_back(Var(c));
			}
			break;
		default:
			assert(0);
			break;
		}
		assert(c < end);
	}
}
