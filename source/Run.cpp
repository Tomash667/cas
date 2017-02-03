#include "Pch.h"
#include "CasImpl.h"
#include "Module.h"
#include "Op.h"
#include "Run.h"

static RunContext ctx;
static vector<Var> stack, global, local;
static Var tmpv;
static vector<StackFrame> stack_frames;
static int current_function, args_offset, locals_offset, current_line;
static uint depth;
static Module* module;
static vector<RefVar*> refs;

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
	assert(v.vartype != V_VOID);
	if(v.vartype == V_STRING)
		v.str->refs++;
	else if(v.vartype.type == V_REF)
		v.ref->refs++;
	else
	{
		Type* type = module->GetType(v.vartype.type);
		if(type->IsClass())
			v.clas->refs++;
	}
}

void ReleaseRef(Var& v)
{
	if(v.vartype == V_STRING)
		v.str->Release();
	else if(v.vartype.type == V_REF)
		v.ref->Release();
	else
	{
		Type* type = module->GetType(v.vartype.type);
		if(type->IsClass())
			v.clas->Release();
	}
}

struct GetRefData
{
	int* data, *real_data;
	VarType vartype;
	bool is_code, ref_to_class;

	inline GetRefData(int* data, VarType vartype, bool is_code = false, bool ref_to_class = false) : data(data), vartype(vartype), is_code(is_code),
		ref_to_class(ref_to_class)
	{
		if(vartype.type == V_STRING && !is_code)
			real_data = (int*)*(Str**)data; // dereference Str** to Str*
		else
			real_data = data;
	}

	template<typename T>
	inline T& as()
	{
		return *(T*)data;
	}
};

GetRefData GetRef(Var& v)
{
	assert(v.vartype.type == V_REF);
	switch(v.ref->type)
	{
	case RefVar::LOCAL:
		if(v.ref->is_valid)
		{
			assert(v.ref->index < local.size());
			Var& vr = local[v.ref->index];
			return GetRefData(&vr.value, vr.vartype);
		}
		else
			return GetRefData(&v.ref->value, VarType(v.vartype.subtype, 0));
	case RefVar::GLOBAL:
		{
			assert(v.ref->index < global.size());
			Var& vr = global[v.ref->index];
			return GetRefData(&vr.value, vr.vartype);
		}
	case RefVar::MEMBER:
		{
			Class* c = v.ref->clas;
			Member* m = c->type->members[v.ref->index];
			return GetRefData((int*)c->at_data(m->offset), m->vartype);
		}
	case RefVar::INDEX:
		{
			Str* s = v.ref->str;
			if(v.ref->index >= s->s.length())
				throw CasException(Format("Index %u out of range.", v.ref->index));
			return GetRefData((int*)&s->s[v.ref->index], V_CHAR);
		}
	case RefVar::CODE:
		return GetRefData(v.ref->adr, VarType(v.vartype.subtype, 0), true, v.ref->ref_to_class);
	default:
		assert(0);
		return GetRefData(nullptr, 0);
	}
}

inline uint alignto(uint size, uint to)
{
	uint n = size / to;
	if(size % to != 0)
		++n;
	return n * to;
}

struct PackedValue
{
	uint offset;
	Type* type;
	union
	{
		Class* c;
		Str* s;
	};

	inline PackedValue(uint offset, Type* type, void* ptr) : offset(offset), type(type), c((Class*)ptr) {}
};

void ExecuteFunction(Function& f)
{
	assert(f.arg_infos.size() < 15u);
	vector<int> packed_args;
	vector<PackedValue> packed_values;
	void* retptr = nullptr;
	bool in_mem = false, ret_by_ref = false;

	// pack return value
	Type* result_type = module->GetType(f.result.type);
	if(f.result.type == V_STRING)
	{
		// string return value
		Str* str = CreateStr();
		// call destructor because function returning string will call ctor
		// this would cause 2x ctor call and memory leak in proxy
		str->s.~basic_string();
		packed_args.push_back((int)(&str->s));
		retptr = str;
	}
	else if(result_type->IsClass())
	{
		// class return value
		if(result_type->IsRefClass())
			ret_by_ref = true;
		else
		{
			Class* c = Class::Create(result_type);
			retptr = c;
			if(result_type->size > 8 || IS_SET(result_type->flags, Type::Complex))
			{
				packed_args.push_back((int)c->data());
				in_mem = true;
			}
		}
	}

	// verify and pack args
	assert(stack.size() >= f.arg_infos.size());
	for(uint i = 0; i < f.arg_infos.size(); ++i)
	{
		Var& v = stack.at(stack.size() - f.arg_infos.size() + i);
		ArgInfo& arg = f.arg_infos[i];
		assert(v.vartype == arg.vartype
			|| (arg.pass_by_ref && v.vartype.type == V_REF && v.vartype.subtype == arg.vartype.type && v.ref->type == RefVar::CODE));
		Type* type;
		bool code_fake_val = (v.vartype != arg.vartype);
		if(code_fake_val || arg.pass_by_ref || !(type = module->GetType(arg.vartype.type))->IsPassByValue())
		{
			int value;
			switch(v.vartype.type)
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
			case V_REF:
				{
					GetRefData refdata = GetRef(v);
					if(refdata.is_code)
					{
						if(code_fake_val)
							assert(v.vartype.subtype == V_STRING);
						else
							assert(IsSimple(v.vartype.subtype));
					}
					else
					{
						assert(!code_fake_val);
						assert(IsSimple(v.vartype.subtype));
					}
					if(refdata.ref_to_class && refdata.vartype.type == V_STRING)
					{
						Str* s = (Str*)refdata.data;
						value = (int)&s->s;
					}
					else
						value = (int)refdata.data;
				}
				break;
			default:
				assert(module->GetType(v.vartype.type)->IsClass());
				value = (int)v.clas->data();
				break;
			}
			packed_args.push_back(value);
		}
		else
		{
			uint size_of;
			void* ptr;
			if(arg.vartype.type == V_STRING)
			{
				size_of = sizeof(string);
				ptr = v.str;
			}
			else
			{
				size_of = type->size;
				ptr = v.clas;
			}
			uint size = alignto(size_of, sizeof(int));
			packed_values.push_back(PackedValue(packed_args.size(), type, ptr));
			packed_args.resize(packed_args.size() + size / sizeof(int));
		}
	}

	// copy string/struct
	for(auto& val : packed_values)
	{
		void* adr = (void*)&packed_args[val.offset];
		if(val.type->index == V_STRING)
			new (adr) string(val.s->s);
		else
			memcpy(adr, val.c->data(), val.type->size);
	}

	// set this
	void* _this;
	int* args;
	uint packed_size;
	uint esp_restore;
	if(!IS_SET(f.flags, CommonFunction::F_THISCALL))
	{
		_this = nullptr;
		args = packed_args.data();
		packed_size = packed_args.size() * 4;
		esp_restore = packed_size;
	}
	else
	{
		_this = (void*)packed_args[0];
		args = packed_args.data() + 1;
		packed_size = packed_args.size() * 4 - 4;
		esp_restore = 0;
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
		mov ecx, packed_size;
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
		add esp, esp_restore;
		lea ecx, result;
		mov[ecx], eax;
		mov 4[ecx], edx;
		fstp fresult;
		pop ecx;
	};
	
	// update stack
	void* passed_result = nullptr;
	for(int i = f.arg_infos.size() - 1; i >= 0; --i)
	{
		Var& v = stack.back();
		ArgInfo& arg = f.arg_infos[i];
		// handle return reference is passed argument
		if(!passed_result && arg.pass_by_ref
			&& ((f.result.type == V_REF && f.result.subtype == arg.vartype.type) || (f.result == arg.vartype && result_type->IsRefClass())))
		{
			if(f.result.subtype == V_STRING)
			{
				if((string*)result.low == &v.str->s)
				{
					v.str->refs++;
					passed_result = v.str;
				}
			}
			else
			{
				if(f.result.type == V_REF)
					assert(module->GetType(f.result.subtype)->IsStruct());
				else
					assert(module->GetType(f.result.type)->IsRefClass());
				if((int*)result.low == v.clas->adr)
				{
					v.clas->refs++;
					passed_result = v.clas;
				}
			}
		}
		ReleaseRef(v);
		stack.pop_back();
	}

	// push result
	switch(f.result.type)
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
	case V_REF:
		{
			RefVar* ref = new RefVar(RefVar::CODE, 0);
			if(passed_result)
			{
				ref->adr = (int*)passed_result;
				ref->ref_to_class = true;
			}
			else
			{
				ref->adr = (int*)result.low;
				Type* real_type = module->GetType(f.result.subtype);
				if(real_type->IsStruct())
				{
					Class* c = Class::CreateCode(real_type, ref->adr);
					ref->adr = (int*)c;
					ref->to_release = true;
				}
			}
			stack.push_back(Var(ref, f.result.subtype));
		}
		break;
	default:
		{
			assert(result_type->IsClass());
			Class* c;
			if(passed_result)
				c = (Class*)passed_result;
			else if(ret_by_ref)
				c = Class::CreateCode(result_type, (int*)result.low);
			else
			{
				c = (Class*)retptr;
				if(!in_mem)
					memcpy(c->data(), &result, c->type->size);
			}
			stack.push_back(Var(c));
		}
		break;
	}
}

void MakeSingleInstance(Var& v)
{
	Type* type = module->GetType(v.vartype.type);
	assert(type && type->IsStruct());
	assert(v.clas->refs >= 1);
	if(v.clas->refs <= 1 && !v.clas->is_code)
		return;
	Class* copy = Class::Copy(v.clas);
	ReleaseRef(v);
	v.clas = copy;
	assert(v.vartype == VarType(v.clas->type->index, 0));
}

// passed by VectorOffset because ReleaseRef can modify vector and invalidate reference
void SetFromStack(VectorOffset<Var>& vo)
{
	Var& v = vo();
	assert(!stack.empty());
	Var& s = stack.back();
	assert(v.vartype == V_VOID || v.vartype == s.vartype);
	if(s.vartype.type >= V_BOOL && s.vartype.type <= V_FLOAT)
		v = s;
	else if(s.vartype.type == V_STRING)
	{
		// free what was in variable previously
		ReleaseRef(v);
		v.vartype = s.vartype;
		v.str = CreateStr(s.str->s.c_str());
	}
	else
	{
		Type* type = module->GetType(v.vartype.type); // with s.vartype.type it will crash for new variables that have type V_VOID
		if(!type->IsStruct())
		{
			// free what was in variable previously
			ReleaseRef(v);
			// incrase reference for new var, ReleaseRef can invalidate s & v
			Var& v = vo();
			Var& s = stack.back();
			AddRef(s);
			v = s;
		}
		else
			memcpy(v.clas->data(), s.clas->data(), type->size);
	}
}

void Cast(Var& v, VarType vartype)
{
	assert(In((CoreVarType)v.vartype.type, { V_BOOL, V_CHAR, V_INT, V_FLOAT, V_STRING }) || module->GetType(v.vartype.type)->IsEnum());
	assert(In((CoreVarType)vartype.type, { V_BOOL, V_CHAR, V_INT, V_FLOAT, V_STRING }) || module->GetType(vartype.type)->IsEnum());
	assert(v.vartype != vartype);

#define COMBINE(x,y) ((x & 0xFF) | ((y & 0xFF) << 8))

	switch(COMBINE(v.vartype.type, vartype.type))
	{
	case COMBINE(V_BOOL, V_CHAR):
		v.cvalue = (v.bvalue ? 't' : 'f');
		v.vartype.type = V_CHAR;
		break;
	case COMBINE(V_BOOL, V_INT):
		v.value = (v.bvalue ? 1 : 0);
		v.vartype.type = V_INT;
		break;
	case COMBINE(V_BOOL, V_FLOAT):
		v.fvalue = (v.bvalue ? 1.f : 0.f);
		v.vartype.type = V_FLOAT;
		break;
	case COMBINE(V_BOOL, V_STRING):
		v.str = CreateStr(v.bvalue ? "true" : "false");
		v.vartype.type = V_STRING;
		break;
	case COMBINE(V_CHAR, V_BOOL):
		v.bvalue = (v.cvalue != 0);
		v.vartype.type = V_BOOL;
		break;
	case COMBINE(V_CHAR, V_INT):
		v.value = (int)v.cvalue;
		v.vartype.type = V_INT;
		break;
	case COMBINE(V_CHAR, V_FLOAT):
		v.fvalue = (float)v.cvalue;
		v.vartype.type = V_FLOAT;
		break;
	case COMBINE(V_CHAR, V_STRING):
		v.str = CreateStr(Format("%c", v.cvalue));
		v.vartype.type = V_STRING;
		break;
	case COMBINE(V_INT, V_BOOL):
		v.bvalue = (v.value != 0);
		v.vartype.type = V_BOOL;
		break;
	case COMBINE(V_INT, V_CHAR):
		v.cvalue = (char)v.value;
		v.vartype.type = V_CHAR;
		break;
	case COMBINE(V_INT, V_FLOAT):
		v.fvalue = (float)v.value;
		v.vartype.type = V_FLOAT;
		break;
	case COMBINE(V_INT, V_STRING):
		v.str = CreateStr(Format("%d", v.value));
		v.vartype.type = V_STRING;
		break;
	case COMBINE(V_FLOAT, V_BOOL):
		v.bvalue = (v.fvalue != 0.f);
		v.vartype.type = V_BOOL;
		break;
	case COMBINE(V_FLOAT, V_CHAR):
		v.cvalue = (char)v.fvalue;
		v.vartype.type = V_CHAR;
		break;
	case COMBINE(V_FLOAT, V_INT):
		v.value = (int)v.fvalue;
		v.vartype.type = V_INT;
		break;
	case COMBINE(V_FLOAT, V_STRING):
		v.str = CreateStr(Format("%g", v.fvalue));
		v.vartype.type = V_STRING;
		break;
	default:
		{
			bool left_enum = module->GetType(v.vartype.type)->IsEnum(),
				right_enum = module->GetType(vartype.type)->IsEnum();
			if(left_enum)
			{
				if(right_enum)
					v.vartype.type = vartype.type;
				else
				{
					v.vartype.type = V_INT;
					if(vartype.type != V_INT)
						Cast(v, vartype);
				}
			}
			else
			{
				if(v.vartype.type != V_INT)
				{
					v.vartype.type = V_INT;
					Cast(v, vartype);
				}
				v.vartype.type = vartype.type;
			}
		}
		break;
	}

#undef COMBINE
}

void RunInternal()
{
	int*& c = ctx.code_pos;

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
				assert(str_index < module->strs.size());
				Str* str = module->strs[str_index];
				str->refs++;
				stack.push_back(Var(str));
			}
			break;
		case PUSH_LOCAL:
			{
				uint local_index = *c++;
				assert(current_function != -1 && (uint)current_function < module->ufuncs.size());
				assert(module->ufuncs[current_function].locals > local_index);
				Var& v = local[locals_offset + local_index];
				AddRef(v);
				stack.push_back(v);
			}
			break;
		case PUSH_LOCAL_REF:
			{
				uint local_index = *c++;
				assert(current_function != -1 && (uint)current_function < module->ufuncs.size());
				assert(module->ufuncs[current_function].locals > local_index);
				uint index = locals_offset + local_index;
				Var& v = local[index];
				assert(v.vartype.type != V_VOID && v.vartype.type != V_REF);
				RefVar* ref = new RefVar(RefVar::LOCAL, index, local_index, depth);
				ref->refs++;
				refs.push_back(ref);
				stack.push_back(Var(ref, v.vartype.type));
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
				assert(v.vartype.type != V_VOID && v.vartype.type != V_REF);
				stack.push_back(Var(new RefVar(RefVar::GLOBAL, global_index), v.vartype.type));
			}
			break;
		case PUSH_ARG:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < module->ufuncs.size());
				assert(module->ufuncs[current_function].args.size() > arg_index);
				Var& v = local[args_offset + arg_index];
				AddRef(v);
				stack.push_back(v);
			}
			break;
		case PUSH_ARG_REF:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < module->ufuncs.size());
				assert(module->ufuncs[current_function].args.size() > arg_index);
				uint index = args_offset + arg_index;
				Var& v = local[index];
				assert(v.vartype.type != V_VOID && v.vartype.type != V_REF);
				RefVar* ref = new RefVar(RefVar::LOCAL, index, ((int)arg_index) - 1, depth);
				ref->refs++;
				refs.push_back(ref);
				stack.push_back(Var(ref, v.vartype.type));
			}
			break;
		case PUSH_MEMBER:
			{
				assert(!stack.empty());
				Var& v = stack.back();
				Type* type = module->GetType(v.vartype.type);
				assert(type->IsClass());
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Member* m = type->members[member_index];
				Class* c = v.clas;
				stack.pop_back();
				switch(m->vartype.type)
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
				Type* type = module->GetType(v.vartype.type);
				assert(type->IsClass());
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Member* m = type->members[member_index];
				assert(m->vartype.type != V_REF);
				Class* c = v.clas;
				stack.pop_back();
				RefVar* ref = new RefVar(RefVar::MEMBER, member_index);
				ref->clas = c;
				stack.push_back(Var(ref, m->vartype.type));
			}
			break;
		case PUSH_THIS_MEMBER:
			{
				// check is inside script class function
				assert(current_function != -1);
				assert((uint)current_function < module->ufuncs.size());
				UserFunction& f = module->ufuncs[current_function];
				Type* type = module->GetType(f.type);
				assert(type->IsClass());
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Var& v = local[args_offset];
				assert(v.vartype.type == f.type);
				Class* c = v.clas;
				Member* m = type->members[member_index];

				// push value
				switch(m->vartype.type)
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
				assert((uint)current_function < module->ufuncs.size());
				UserFunction& f = module->ufuncs[current_function];
				Type* type = module->GetType(f.type);
				assert(type->IsClass());
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Var& v = local[args_offset];
				assert(v.vartype.type == f.type);
				Class* c = v.clas;
				++c->refs;
				Member* m = type->members[member_index];

				// push reference
				assert(m->vartype.type != V_REF);
				RefVar* ref = new RefVar(RefVar::MEMBER, member_index);
				ref->clas = c;
				stack.push_back(Var(ref, m->vartype.type));
			}
			break;
		case PUSH_TMP:
			stack.push_back(tmpv);
			break;
		case PUSH_INDEX:
			{
				assert(stack.size() >= 2u);
				Var vindex = stack.back();
				assert(vindex.vartype.type == V_INT);
				uint index = vindex.value;
				stack.pop_back();
				Var& v = stack.back();
				assert(v.vartype.type == V_STRING);
				if(index >= v.str->s.length())
					throw CasException(Format("Index %u out of range.", index));
				RefVar* ref = new RefVar(RefVar::INDEX, index);
				ref->str = v.str;
				v.vartype = VarType(V_REF, V_CHAR);
				v.ref = ref;
			}
			break;
		case PUSH_THIS:
			{
				assert(current_function != -1);
				assert((uint)current_function < module->ufuncs.size());
				UserFunction& f = module->ufuncs[current_function];
				Type* type = module->GetType(f.type);
				assert(type->IsClass());
				Var& v = local[args_offset];
				AddRef(v);
				stack.push_back(v);
			}
			break;
		case PUSH_ENUM:
			{
				int type_idx = *c++;
				int value = *c++;
				assert(module->GetType(type_idx)->IsEnum());
				stack.push_back(Var(VarType(type_idx, 0), value));
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
				assert(current_function != -1 && (uint)current_function < module->ufuncs.size());
				assert(module->ufuncs[current_function].locals > local_index);
				SetFromStack(VectorOffset<Var>(local, locals_offset + local_index));
			}
			break;
		case SET_GLOBAL:
			{
				uint global_index = *c++;
				assert(global_index < global.size());
				SetFromStack(VectorOffset<Var>(global, global_index));
			}
			break;
		case SET_ARG:
			{
				uint arg_index = *c++;
				assert(current_function != -1 && (uint)current_function < module->ufuncs.size());
				assert(module->ufuncs[current_function].args.size() > arg_index);
				SetFromStack(VectorOffset<Var>(local, args_offset + arg_index));
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
				Type* type = module->GetType(cv.vartype.type);
				assert(type->IsClass());
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Member* m = type->members[member_index];
				assert(v.vartype.type == m->vartype.type);
				Class* c = cv.clas;

				switch(m->vartype.type)
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
				assert((uint)current_function < module->ufuncs.size());
				UserFunction& f = module->ufuncs[current_function];
				Type* type = module->GetType(f.type);
				assert(type->IsClass());
				uint member_index = *c++;
				assert(member_index < type->members.size());
				Var& vl = local[args_offset];
				assert(vl.vartype.type == f.type);
				Class* c = vl.clas;
				Member* m = type->members[member_index];

				switch(m->vartype.type)
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
				Cast(v, VarType(type, 0));
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
					assert(v.vartype.type == V_INT || v.vartype.type == V_FLOAT);
					if(v.vartype.type == V_INT)
						v.value = -v.value;
					else
						v.fvalue = -v.fvalue;
				}
				else if(op == NOT)
				{
					assert(v.vartype.type == V_BOOL);
					v.bvalue = !v.bvalue;
				}
				else if(op == BIT_NOT)
				{
					assert(v.vartype.type == V_INT);
					v.value = ~v.value;
				}
				else if(op == DEREF)
				{
					auto data = GetRef(v);
					int value;
					if(data.is_code)
					{
						if(data.vartype.type == V_STRING)
						{
							if(data.ref_to_class)
								value = (int)(Str*)data.data;
							else
								value = (int)CreateStr(((string*)data.data)->c_str());
						}
						else
						{
							Type* type = module->GetType(data.vartype.type);
							if(type->IsStruct())
								value = (int)(Class*)data.data;
							else
							{
								assert(type->IsSimple());
								value = *data.data;
							}
						}
					}
					else
						value = *data.data;
					Var nv;
					nv.value = value;
					nv.vartype = data.vartype;
					AddRef(nv);
					ReleaseRef(v);
					v = nv;
				}
				else
				{
					assert(v.vartype.type == V_CHAR || v.vartype.type == V_INT || v.vartype.type == V_FLOAT);
					if(op == INC)
					{
						switch(v.vartype.type)
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
						switch(v.vartype.type)
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
					assert(left.vartype.type == right.vartype.type);
				if(op == ADD)
					assert(left.vartype.type == V_INT || left.vartype.type == V_FLOAT || left.vartype.type == V_STRING);
				else if(op == EQ || op == NOT_EQ)
					assert(left.vartype.type == V_BOOL || left.vartype.type == V_CHAR || left.vartype.type == V_INT || left.vartype.type == V_FLOAT
						|| left.vartype.type == V_STRING || module->GetType(left.vartype.type)->IsClass()
						|| module->GetType(left.vartype.type)->IsEnum());
				else if(op == AND || op == OR)
					assert(left.vartype.type == V_BOOL);
				else if(op == BIT_AND || op == BIT_OR || op == BIT_XOR || op == BIT_LSHIFT || op == BIT_RSHIFT)
					assert(left.vartype.type == V_INT);
				else if(op == IS)
					assert(left.vartype.type == V_STRING || module->GetType(left.vartype.type)->IsClass() || left.vartype.type == V_REF);
				else if(op == SET_ADR)
					assert(left.vartype.type == V_REF);
				else
					assert(left.vartype.type == V_INT || left.vartype.type == V_FLOAT);

				switch(op)
				{
				case ADD:
					if(left.vartype.type == V_INT)
						left.value += right.value;
					else if(left.vartype.type == V_FLOAT)
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
					if(left.vartype.type == V_INT)
						left.value -= right.value;
					else
						left.fvalue -= right.fvalue;
					break;
				case MUL:
					if(left.vartype.type == V_INT)
						left.value *= right.value;
					else
						left.fvalue *= right.fvalue;
					break;
				case DIV:
					if(left.vartype.type == V_INT)
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
					if(left.vartype.type == V_INT)
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
					switch(left.vartype.type)
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
					default:
						{
							bool result = (left.clas == right.clas);
							ReleaseRef(left);
							ReleaseRef(right);
							left.bvalue = result;
						}
						break;
					}
					left.vartype.type = V_BOOL;
					break;
				case NOT_EQ:
					switch(left.vartype.type)
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
					default:
						{
							bool result = (left.clas != right.clas);
							ReleaseRef(left);
							ReleaseRef(right);
							left.bvalue = result;
						}
						break;
					}
					left.vartype.type = V_BOOL;
					break;
				case GR:
					if(left.vartype.type == V_INT)
						left.bvalue = (left.value > right.value);
					else
						left.bvalue = (left.fvalue > right.fvalue);
					left.vartype.type = V_BOOL;
					break;
				case GR_EQ:
					if(left.vartype.type == V_INT)
						left.bvalue = (left.value >= right.value);
					else
						left.bvalue = (left.fvalue >= right.fvalue);
					left.vartype.type = V_BOOL;
					break;
				case LE:
					if(left.vartype.type == V_INT)
						left.bvalue = (left.value < right.value);
					else
						left.bvalue = (left.fvalue < right.fvalue);
					left.vartype.type = V_BOOL;
					break;
				case LE_EQ:
					if(left.vartype.type == V_INT)
						left.bvalue = (left.value <= right.value);
					else
						left.bvalue = (left.fvalue <= right.fvalue);
					left.vartype.type = V_BOOL;
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
						if(left.vartype.type == V_STRING)
							result = (left.str == right.str);
						else if(left.vartype.type == V_REF)
						{
							GetRefData refl = GetRef(left);
							GetRefData refr = GetRef(right);
							result = (refl.real_data == refr.real_data);
						}
						else
							result = (left.clas == right.clas);
						ReleaseRef(right);
						ReleaseRef(left);
						left.vartype = VarType(V_BOOL, 0);
						left.bvalue = result;
					}
					break;
				case SET_ADR:
					{
						GetRefData ref = GetRef(left);
						assert(ref.vartype.type == right.vartype.type);
						if(ref.is_code)
						{
							if(ref.vartype.type == V_STRING)
							{
								if(ref.ref_to_class)
								{
									Str* s = (Str*)ref.data;
									s->s = right.str->s;
								}
								else
								{
									string& s = *(string*)ref.data;
									s = right.str->s;
								}
							}
							else
							{
								Type* type = module->GetType(right.vartype.type);
								assert(type->IsSimple());
								memcpy(ref.data, &right.value, type->size);
							}							
						}
						else if(ref.vartype.type == V_STRING)
						{
							Str* str = *(Str**)ref.data;
							str->s = right.str->s;
						}
						else
						{
							Type* type = module->GetType(right.vartype.type);
							uint size;
							if(type->IsClass())
							{
								assert(!type->IsStruct());
								Class* lclass = (Class*)*ref.data;
								lclass->Release();
								right.clas->refs++;
								size = sizeof(lclass);
							}
							else
								size = type->size;
							memcpy(ref.data, &right.value, size);
						}
						ReleaseRef(left);
						stack.pop_back();
						stack.push_back(right);
					}
					break;
				}
			}
			break;
		case RET:
			// return from function/main
			if(current_function == -1)
			{
				// set & validate return value
				assert(depth == 0);
				assert(local.empty());
				if(stack.empty())
				{
					// set void as return value
					module->return_value.int_value = 0;
					module->return_value.type = module->GetType(V_VOID);
				}
				else
				{
					// set return value
					assert(stack.size() == 1u);
					Var& v = stack.back();
					module->return_value.type = module->GetType(v.vartype.type);
					module->return_value.int_value = v.value;
					stack.pop_back();
				}
				return;
			}
			else
			{
				assert((uint)current_function < module->ufuncs.size());
				UserFunction& f = module->ufuncs[current_function];
				uint to_pop = f.locals + f.args.size();
				assert(local.size() > to_pop);
				Var& func_mark = *(local.end() - to_pop - 1);
				assert(func_mark.vartype.type == V_SPECIAL);
				while(!refs.empty())
				{
					RefVar* ref = refs.back();
					if(ref->depth != depth)
						break;
					if(ref->refs != 1)
					{
						assert(ref->index < local.size());
						Var& vr = local[ref->index];
						ref->is_valid = false;
						ref->value = vr.value;
					}
					ref->Release();
					refs.pop_back();
				}
				--depth;
				if(stack_frames.back().type == StackFrame::CTOR)
					--to_pop;
				int tmp_cleanup_offset = ctx.cleanup_offset;
				ctx.cleanup_offset = 0;
				while(to_pop--)
				{
					Var& v = local.back();
					ReleaseRef(v);
					local.pop_back();
					ctx.cleanup_offset++;
				}
				ctx.cleanup_offset = tmp_cleanup_offset;
				StackFrame& frame = stack_frames.back();
				Class* thi = nullptr;
				if(frame.type == StackFrame::CTOR)
				{
					assert(local.back().vartype.type == f.type);
					thi = local.back().clas;
					local.pop_back();
				}
				c = ctx.code_start + frame.pos;
				current_function = frame.current_function;
				local.pop_back();
				if(current_function != -1)
				{
					// checking local stack
					assert((uint)current_function < module->ufuncs.size());
					UserFunction& f = module->ufuncs[current_function];
					uint count = 1 + f.locals + f.args.size();
					assert(local.size() >= count - ctx.cleanup_offset);
					Var& d = *(local.end() - (count - ctx.cleanup_offset));
					assert(d.vartype.type == V_SPECIAL);
					locals_offset = local.size() - f.locals - ctx.cleanup_offset;
					args_offset = locals_offset - f.args.size() - ctx.cleanup_offset;
				}
				if(thi)
					stack.push_back(Var(thi));
				assert(frame.expected_stack == stack.size());
				if(f.result.type != V_VOID)
					assert(stack.back().vartype == f.result);
				current_line = frame.current_line;
				if(frame.type == StackFrame::DTOR)
				{
					stack_frames.pop_back();
					return;
				}
				stack_frames.pop_back();
			}
			break;
		case JMP:
			{
				uint offset = *c;
				c = ctx.code_start + offset;
				assert(c < ctx.code_end);
			}
			break;
		case TJMP:
		case FJMP:
			{
				uint offset = *c++;
				int* new_c = ctx.code_start + offset;
				assert(new_c < ctx.code_end);
				assert(!stack.empty());
				Var v = stack.back();
				stack.pop_back();
				assert(v.vartype.type == V_BOOL);
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
				Function* f = module->GetFunction(f_idx);
				ExecuteFunction(*f);
			}
			break;
		case CALLU:
			{
				uint f_idx = *c++;
				assert(f_idx < module->ufuncs.size());
				UserFunction& f = module->ufuncs[f_idx];
				// mark function call
				uint pos = c - ctx.code_start;
				local.push_back(Var(V_SPECIAL));
				// handle args
				assert(stack.size() >= f.args.size());
				args_offset = local.size();
				local.resize(local.size() + f.args.size());
				for(uint i = 0, count = f.args.size(); i < count; ++i)
				{
					assert(stack.back().vartype == f.args[count - 1 - i]);
					local[args_offset + count - 1 - i] = stack.back();
					stack.pop_back();
				}
				// handle locals
				locals_offset = local.size();
				local.resize(local.size() + f.locals);
				// push frame
				uint expected = stack.size();
				if(f.result.type != V_VOID)
					++expected;
				StackFrame frame;
				frame.pos = pos;
				frame.current_function = current_function;
				frame.current_line = current_line;
				frame.expected_stack = expected;
				frame.type = StackFrame::NORMAL;
				stack_frames.push_back(frame);
				// jmp to new location
				current_function = f_idx;
				current_line = -1;
				c = ctx.code_start + f.pos;
				++depth;
			}
			break;
		case CALLU_CTOR:
			{
				uint f_idx = *c++;
				assert(f_idx < module->ufuncs.size());
				UserFunction& f = module->ufuncs[f_idx];
				// mark function call
				uint pos = c - ctx.code_start;
				local.push_back(Var(V_SPECIAL));
				// push this
				args_offset = local.size();
				assert(module->GetType(f.type)->IsClass());
				local.resize(local.size() + f.args.size());
				local[args_offset] = Var(Class::Create(module->GetType(f.type)));
				// handle args
				assert(stack.size() >= f.args.size() - 1);
				for(uint i = 1, count = f.args.size(); i < count; ++i)
				{
					assert(stack.back().vartype == f.args[count - i]);
					local[args_offset + count - i] = stack.back();
					stack.pop_back();
				}
				// handle locals
				locals_offset = local.size();
				local.resize(local.size() + f.locals);
				// push frame
				uint expected = stack.size() + 1;
				StackFrame frame;
				frame.pos = pos;
				frame.current_function = current_function;
				frame.current_line = current_line;
				frame.expected_stack = expected;
				frame.type = StackFrame::CTOR;
				stack_frames.push_back(frame);
				// jmp to new location
				current_function = f_idx;
				current_line = -1;
				c = ctx.code_start + f.pos;
				++depth;
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
				assert(current_function != -1 && (uint)current_function < module->ufuncs.size());
				assert(module->ufuncs[current_function].args.size() > arg_index);
				Var& v = local[args_offset + arg_index];
				MakeSingleInstance(v);
			}
			break;
		case RELEASE_REF:
			{
				int index = *c++;
#ifdef _DEBUG
				bool any = false;
#endif
				for(int i = refs.size() - 1; i >= 0; --i)
				{
					RefVar* ref = refs[i];
					if(ref->depth != depth)
						break;
					if(ref->var_index == index)
					{
						if(ref->refs != 1)
						{
							assert(ref->index < local.size());
							Var& vr = local[ref->index];
							ref->is_valid = false;
							ref->value = vr.value;
						}
						ref->Release();
						refs.erase(refs.begin() + i);
						DEBUG_DO(any = true);
					}
				}
				assert(any);
			}
			break;
		case LINE:
			current_line = *c++;
			break;
		default:
			assert(0);
			break;
		}
		assert(c < ctx.code_end);
	}
}

bool Run(Module& _module)
{
	module = &_module;

	// prepare stack
	tmpv = Var();
	stack.clear();
	global.clear();
	global.resize(module->globals);
	local.clear();
	current_function = -1;
	depth = 0;
	current_line = -1;
	stack_frames.clear();
#ifdef CHECK_LEAKS
	all_classes.clear();
	all_refs.clear();
#endif
	ctx.code_start = module->code.data();
	ctx.code_end = ctx.code_start + module->code.size();
	ctx.code_pos = ctx.code_start + module->entry_point;
	ctx.cleanup_offset = 0;

	bool result;
	try
	{
		// run
		RunInternal();
		module->exc.clear();
		result = true;

		// cleanup
		for(Var& v : global)
			ReleaseRef(v);
#ifdef CHECK_LEAKS
		assert(all_refs.empty());
		assert(all_classes.empty());
#endif
	}
	catch(const CasException& ex)
	{
		module->exc = ex.exc;
		result = false;
	}

	return result;
}

std::pair<cstring, int> cas::GetCurrentLocation()
{
	cstring func_name;
	if(current_function == -1)
		func_name = "<global>";
	else
		func_name = module->ufuncs[current_function].name.c_str();
	return std::pair<cstring, uint>(func_name, current_line);
}

void CallSimpleFunction(Function* f, void* _this)
{
	void* clbk = f->clbk;
	int to_push = IS_SET(f->flags, CommonFunction::F_THISCALL) ? 4 : 0;

	__asm
	{
		// copy old ecx
		push ecx;
		// prepare args
		mov ecx, _this;
		mov eax, to_push;
		cmp eax, 0;
		je end;
		push ecx;
	end:
		// call
		call clbk;
		// restore
		add esp, to_push;
		pop ecx;
	};
}

void ReleaseClass(Class* c, bool dtor)
{
	if(dtor)
	{
		assert(c->type->dtor);
		if(c->type->dtor.IsScript())
		{
			// incrase reference to prevent infinite release loop
			c->refs = 2;

			StackFrame frame;
			frame.current_function = current_function;
			frame.current_line = current_line;
			frame.expected_stack = stack.size();
			frame.type = StackFrame::DTOR;
			frame.pos = ctx.code_pos - ctx.code_start;
			stack_frames.push_back(frame);

			UserFunction& f = *c->type->dtor.uf;
			local.push_back(Var(V_SPECIAL));

			// push this
			args_offset = local.size();
			local.push_back(Var(c));
			// handle locals
			locals_offset = local.size();
			local.resize(local.size() + f.locals);
			// jmp to new location
			current_function = c->type->dtor.uf->index;
			current_line = -1;
			ctx.code_pos = ctx.code_start + f.pos;
			++depth;

			RunInternal();
		}
		else
			CallSimpleFunction(c->type->dtor.f, c->adr);
	}
	else
	{
		Function* f = c->type->FindSpecialCodeFunction(SF_RELEASE);
		assert(f);
		CallSimpleFunction(f, c->adr);
	}
}
