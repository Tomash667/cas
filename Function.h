#pragma once

#include "Var.h"

typedef void(*VoidF)();

struct Function
{
	cstring name;
	VoidF clbk;
	int index;
	VAR_TYPE result, var_type;
	vector<VAR_TYPE> args;
};

extern vector<Function> functions;

inline Function* FindFunction(const string& id, VAR_TYPE var_type = V_VOID)
{
	for(Function& f : functions)
	{
		if(id == f.name && var_type == f.var_type)
			return &f;
	}
	return nullptr;
}

void RegisterFunctions();
