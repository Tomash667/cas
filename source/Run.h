#pragma once

#ifdef _DEBUG
#define CHECK_LEAKS
#endif

struct Class;

#ifdef CHECK_LEAKS
struct RefVar;
static vector<Class*> all_classes;
static vector<RefVar*> all_refs;
static const int START_REF_COUNT = 2;
#else
static const int START_REF_COUNT = 1;
#endif

void ReleaseClass(Class* c);

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
		c->refs = START_REF_COUNT;
		c->type = type;
		c->is_code = false;
		c->adr = ((int*)&c->adr) + 1;
		memset(c->adr, 0, type->size);
#ifdef CHECK_LEAKS
		all_classes.push_back(c);
#endif
		return c;
	}

	inline static Class* CreateCode(Type* type, int* real_class)
	{
		assert(type);
		Class* c = new Class;
		c->refs = START_REF_COUNT;
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
		c->refs = START_REF_COUNT;
		c->is_code = false;
		c->adr = ((int*)&c->adr) + 1;
		memcpy(c->adr, base->adr, type->size);
#ifdef CHECK_LEAKS
		all_classes.push_back(c);
#endif
		return c;
	}

	inline void Release()
	{
		if(--refs == 0)
		{
#ifdef CHECK_LEAKS
			assert(0); // there should be at last 1 reference
#endif
			Delete();
		}
	}

	inline void Delete()
	{
		if(is_code)
		{
			if(IS_SET(type->flags, Type::RefCount))
				ReleaseClass(this);
			delete this;
		}
		else
		{
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
	bool is_valid, to_release;

	// hopefuly noone will use function with 999 args
	inline RefVar(Type type, uint index, int var_index = -999, uint depth = 0) : type(type), refs(START_REF_COUNT), index(index), var_index(var_index),
		depth(depth), is_valid(true), to_release(false)
	{
#ifdef CHECK_LEAKS
		all_refs.push_back(this);
#endif
	}

	inline ~RefVar()
	{
		if(type == MEMBER)
			clas->Release();
		else if(type == INDEX)
			str->Release();
		else if(type == CODE && to_release)
			clas->Release();
	}

	inline void Release()
	{
		if(--refs == 0)
		{
#ifdef CHECK_LEAKS
			assert(0); // there should be at last 1 reference
#endif
			delete this;
		}
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
};

struct StackFrame
{
	uint expected_stack, pos;
	int current_line, current_function;
	bool is_ctor;
};

struct CasException
{
	cstring exc;
	CasException(cstring exc) : exc(exc) {}
};
