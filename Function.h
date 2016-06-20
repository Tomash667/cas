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

struct UserFunction
{
	uint pos;
	uint locals;
#ifdef _DEBUG
	vector<VAR_TYPE> args;
#else
	uint args;
#endif
	VAR_TYPE result;

	inline uint GetArgs() const
	{
#ifdef _DEBUG
		return args.size();
#else
		return args;
#endif
	}
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
