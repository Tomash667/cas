#pragma once

#include "Var.h"

typedef void(*VoidF)();

struct Function
{
	cstring name;
	VoidF clbk;
	int index;
	VAR_TYPE result;
	vector<VAR_TYPE> args;
};

extern vector<Function> functions;

inline Function* FindFunction(const string& id)
{
	for(Function& f : functions)
	{
		if(id == f.name)
			return &f;
	}
	return nullptr;
}

void RegisterFunctions();
