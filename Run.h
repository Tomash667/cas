#pragma once

#include "Var.h"
#include "Function.h"

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
			int value1;
			int value2;
		};
	};

	inline explicit Var() : type(V_VOID) {}
	inline explicit Var(bool bvalue) : type(V_BOOL), bvalue(bvalue) {}
	inline explicit Var(int value) : type(V_INT), value(value) {}
	inline explicit Var(float fvalue) : type(V_FLOAT), fvalue(fvalue) {}
	inline explicit Var(Str* str) : type(V_STRING), str(str) {}
	inline explicit Var(int type, int value1, int value2=0) : type(type), value1(value1), value2(value2) {}
};

extern vector<Var> stack;

struct RunContext
{
	vector<int> code;
	vector<string> strs;
	vector<UserFunction> ufuncs;
	uint globals, entry_point;
};

void RunCode(RunContext& ctx);
