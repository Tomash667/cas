#pragma once

enum VAR_TYPE
{
	V_VOID,
	V_BOOL,
	V_INT,
	V_FLOAT,
	V_STRING,
	V_MAX
};

struct VarInfo
{
	VAR_TYPE type;
	cstring name;
	bool reg;
};

struct Str : ObjectPoolProxy<Str>
{
	string s;
	int refs;

	inline void Release()
	{
		--refs;
		if(refs == 0)
			Free();
	}
};

struct ArgInfo
{
	VAR_TYPE type;
	union
	{
		bool bvalue;
		int value;
		float fvalue;
	};
	bool have_def_value;

	ArgInfo(VAR_TYPE type) : type(type), have_def_value(false) {}
	ArgInfo(bool bvalue) : type(V_BOOL), bvalue(bvalue), have_def_value(true) {}
	ArgInfo(int value) : type(V_INT), value(value), have_def_value(true) {}
	ArgInfo(float fvalue) : type(V_FLOAT), fvalue(fvalue), have_def_value(true) {}
	ArgInfo(VAR_TYPE type, int value) : type(type), value(value), have_def_value(true) {}
};

extern VarInfo var_info[V_MAX];
