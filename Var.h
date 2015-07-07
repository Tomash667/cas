#pragma once

enum VarType
{
	VOID,
	INT,
	STR,
	BOOL,
	FLOAT,
	REF,
	ARRAY,
	RETURN_ADDRESS,
	LOCALS_MARKER
};

inline cstring VarTypeToString(VarType type)
{
	switch(type)
	{
	case VOID:
		return "void";
	case INT:
		return "int";
	case STR:
		return "string";
	case BOOL:
		return "bool";
	case FLOAT:
		return "float";
	case REF:
		return "ref";
	case ARRAY:
		return "array";
	case RETURN_ADDRESS:
		return "return address";
	case LOCALS_MARKER:
		return "locals marker";
	default:
		assert(0);
		return "???";
	}
}

struct Str
{
	string s;
	int refs;
};

struct _StrPool
{
	vector<Str*> v;

	inline Str* Get()
	{
		Str* s;
		if(!v.empty())
		{
			s = v.back();
			v.pop_back();
		}
		else
			s = new Str;
		return s;
	}

	inline void Free(Str* s)
	{
		v.push_back(s);
	}
};
extern _StrPool StrPool;

union VarValue
{
	int i;
	float f;
	Str* str;
	bool b;
	vector<VarValue>* arr;
};

struct Var
{
	VarValue v;
	VarType type;
	union
	{
		VarType subtype;
		int prev_func;
	};
	int offset;

	inline Var() {}
	inline Var(VarType type) : type(type) {}
	inline Var(int value) : type(INT)
	{
		v.i = value;
	}
	inline Var(float f) : type(FLOAT)
	{
		v.f = f;
	}
	inline Var(string& s) : type(STR)
	{
		v.str = StrPool.Get();
		v.str->refs = 1;
		v.str->s = s;
	}
	inline Var(Str* s) : type(STR)
	{
		v.str = s;
	}
	/*inline Var(const Var& va, int copy) : type(va.type)
	{
		v.i = va.v.i;
		if(type == STR)
			v.str->refs++;
	}*/
	inline Var(VarType type, VarValue val) : type(type)
	{
		v.i = val.i;
		if(type == STR)
			v.str->refs++;
	}

	inline void Clean()
	{
		if(type == STR && --v.str->refs == 0)
		{
			StrPool.Free(v.str);
#ifdef _DEBUG
			v.str = NULL;
#endif
		}
	}

	inline void Clean(uint index)
	{
		assert(type == ARRAY && v.arr->size() > index);
		if(subtype == STR)
		{
			VarValue& e = v.arr->at(index);
			if(--e.str->refs == 0)
			{
				StrPool.Free(e.str);
#ifdef _DEBUG
				e.str = NULL;
#endif
			}
		}
	}

	Var Copy()
	{
		Var va(type);
		va.v.i = v.i;
		if(type == STR)
			v.str->refs++;
		return va;
	}
};
