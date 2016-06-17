#pragma once

#include "Var.h"

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
	VAR_TYPE type;
	union
	{
		bool bvalue;
		int value;
		float fvalue;
		Str* str;
	};

	inline explicit Var() : type(V_VOID) {}
	inline explicit Var(bool bvalue) : type(V_BOOL), bvalue(bvalue) {}
	inline explicit Var(int value) : type(V_INT), value(value) {}
	inline explicit Var(float fvalue) : type(V_FLOAT), fvalue(fvalue) {}
	inline explicit Var(Str* str) : type(V_STRING), str(str) {}
	inline explicit Var(VAR_TYPE type, int value) : type(type), value(value) {}
};

extern vector<Var> stack;

void RunCode(vector<int>& code, vector<string>& strs, uint n_vars);
