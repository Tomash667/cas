#pragma once

#ifdef _DEBUG
#define CHECK_LEAKS
#endif

enum REF_TYPE
{
	REF_GLOBAL,
	REF_LOCAL,
	REF_MEMBER,
	REF_CODE
};

enum SPECIAL_VAR
{
	V_FUNCTION,
	V_CTOR
};

#ifdef CHECK_LEAKS
struct Class;
static vector<Class*> all_clases;
static const int START_REF_COUNT = 2;
#else
static const int START_REF_COUNT = 1;
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
		c->refs = START_REF_COUNT;
		c->type = type;
#ifdef CHECK_LEAKS
		all_clases.push_back(c);
#endif
		return c;
	}

	inline static Class* Copy(Class* base)
	{
		assert(base);
		Type* type = base->type;
		byte* data = new byte[type->size + 8];
		memcpy(data + 8, base, type->size);
		Class* c = (Class*)data;
		c->refs = START_REF_COUNT;
		c->type = type;
#ifdef CHECK_LEAKS
		all_clases.push_back(c);
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
			delete this;
		}
	}
};

struct Var
{
	int type;
	union
	{
		bool bvalue;
		char cvalue;
		int value;
		float fvalue;
		Str* str;
		struct
		{
			REF_TYPE ref_type;
			union
			{
				struct
				{
					Class* ref_class;
					uint ref_index;
				};
				struct
				{
					int* ref_adr;
					int ref_var_type;
				};
			};

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
	inline explicit Var(char cvalue) : type(V_CHAR), cvalue(cvalue) {}
	inline explicit Var(int value) : type(V_INT), value(value) {}
	inline explicit Var(float fvalue) : type(V_FLOAT), fvalue(fvalue) {}
	inline explicit Var(Str* str) : type(V_STRING), str(str) {}
	inline Var(REF_TYPE ref_type, uint ref_index, Class* ref_class) : type(V_REF), ref_type(ref_type), ref_index(ref_index), ref_class(ref_class) {}
	inline explicit Var(Class* clas) : type(clas->type->index), clas(clas) {}
	inline Var(int type, int special_type, int value1, int value2) : type(type), special_type(special_type), value1(value1), value2(value2) {}
};
