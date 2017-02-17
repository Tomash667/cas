#pragma once

#ifdef _DEBUG
#define CHECK_LEAKS
#endif

struct Class;

#ifdef CHECK_LEAKS
struct RefVar;
static vector<Class*> all_classes;
static vector<RefVar*> all_refs;
#endif

void ReleaseClass(Class* c, bool dtor);

// For class created in script this Class have larger size and class data is stored in it, starting at adr
// for code class, adr points to class created in code
struct Class
{
	int refs;
	Type* type;
	bool is_code;
	int* adr;

	inline int* data()
	{
		return adr;
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
		byte* data = new byte[type->size + sizeof(Class)];
		Class* c = (Class*)data;
		c->refs = 1;
		c->type = type;
		c->is_code = false;
		c->adr = ((int*)&c->adr) + 1;
		memset(c->adr, 0, type->size);
		if(type->have_complex_member && type->IsCode())
		{
			for(Member* m : type->members)
			{
				if(m->vartype.type == V_STRING)
				{
					string* str = &c->at<string>(m->offset);
					new(str) string;
				}
			}
		}
#ifdef CHECK_LEAKS
		all_classes.push_back(c);
#endif
		return c;
	}

	inline static Class* CreateCode(Type* type, int* real_class)
	{
		assert(type);
		Class* c = new Class;
		c->refs = 1;
		c->type = type;
		c->is_code = true;
		c->adr = real_class;
#ifdef CHECK_LEAKS
		all_classes.push_back(c);
#endif
		return c;
	}

	inline static Class* Copy(Class* base)
	{
		assert(base);
		Type* type = base->type;
		byte* data = new byte[type->size + sizeof(Class)];
		Class* c = (Class*)data;
		c->type = type;
		c->refs = 1;
		c->is_code = false;
		c->adr = ((int*)&c->adr) + 1;
		memcpy(c->adr, base->adr, type->size);
		if(type->have_complex_member)
		{
			for(Member* m : type->members)
			{
				if(m->vartype.type == V_STRING)
				{
					Str* str = c->at<Str*>(m->offset);
					str->refs++;
				}
			}
		}
#ifdef CHECK_LEAKS
		all_classes.push_back(c);
#endif
		return c;
	}

	inline bool Release()
	{
		assert(refs >= 1);
		if(--refs == 0)
		{
#ifdef CHECK_LEAKS
			RemoveElement(all_classes, this);
#endif
			Delete();
			return true;
		}
		else
			return false;
	}

	inline void Delete()
	{
		bool mem_free = false;
		if(type->dtor)
		{
			if(is_code && !type->IsStruct())
				mem_free = true;
			ReleaseClass(this, true);
		}
		if(is_code)
		{
			if(type->IsRefCounted())
				ReleaseClass(this, false);
			if(!mem_free)
				delete this;
		}
		else if(!mem_free)
		{
			if(type->have_complex_member)
			{
				for(Member* m : type->members)
				{
					if(m->vartype.type == V_STRING)
					{
						if(!type->IsCode())
						{
							Str* str = at<Str*>(m->offset);
							str->Release();
						}
						else
						{
							string& str = at<string>(m->offset);
							str.~basic_string();
						}
					}
				}
			}
			byte* data = (byte*)this;
			delete[] data;
		}
	}
};

struct RefVar
{
	enum Type
	{
		LOCAL,
		GLOBAL,
		MEMBER,
		INDEX,
		CODE
	};

	Type type;
	int refs, var_index, value;
	uint index, depth;
	union
	{
		Class* clas;
		Str* str;
		int* adr;
	};
	bool is_valid, to_release, ref_to_class;

	// hopefuly noone will use function with 999 args
	inline RefVar(Type type, uint index, int var_index = -999, uint depth = 0) : type(type), refs(1), index(index), var_index(var_index), depth(depth),
		is_valid(true), to_release(false), ref_to_class(false)
	{
#ifdef CHECK_LEAKS
		all_refs.push_back(this);
#endif
	}

	inline ~RefVar()
	{
		if(type == MEMBER || ref_to_class)
			clas->Release();
		else if(type == INDEX)
			str->Release();
		else if(type == CODE && to_release)
			clas->Release();
	}

	inline bool Release()
	{
		assert(refs >= 1);
		if(--refs == 0)
		{
#ifdef CHECK_LEAKS
			RemoveElement(all_refs, this);
#endif
			delete this;
			return true;
		}
		else
			return false;
	}
};

struct Var
{
	VarType vartype;
	union
	{
		bool bvalue;
		char cvalue;
		int value;
		float fvalue;
		Str* str;
		RefVar* ref;
		Class* clas;
	};

	inline explicit Var() : vartype(V_VOID) {}
	inline explicit Var(CoreVarType type) : vartype(type) {}
	inline explicit Var(bool bvalue) : vartype(V_BOOL), bvalue(bvalue) {}
	inline explicit Var(char cvalue) : vartype(V_CHAR), cvalue(cvalue) {}
	inline explicit Var(int value) : vartype(V_INT), value(value) {}
	inline explicit Var(float fvalue) : vartype(V_FLOAT), fvalue(fvalue) {}
	inline explicit Var(Str* str) : vartype(V_STRING), str(str) {}
	inline Var(RefVar* ref, int subtype) : vartype(VarType(V_REF, subtype)), ref(ref) {}
	inline explicit Var(Class* clas) : vartype(clas->type->index, 0), clas(clas) {}
	inline Var(VarType vartype, int value) : vartype(vartype), value(value) {}
};

struct StackFrame
{
	enum Type
	{
		NORMAL,
		CTOR,
		DTOR
	};

	uint expected_stack, pos;
	int current_line, current_function;
	Type type;
};

struct RunContext
{
	int* code_start;
	int* code_end;
	int* code_pos;
	int cleanup_offset;
};

template<typename T>
struct VectorOffset
{
	inline VectorOffset(vector<T>& _data, uint offset) : data(&_data), offset(offset)
	{

	}

	inline T* operator -> ()
	{
		return &data->at(offset);
	}

	inline T& operator () ()
	{
		return data->at(offset);
	}

	vector<T>* data;
	uint offset;
};
