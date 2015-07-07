#pragma once

#include "Var.h"

struct FunctionInfo
{
	cstring name;
	void(*ptr)();
	VarType result;
	VarType params[4];
	uint params_count;
};

extern vector<FunctionInfo> functions;

void register_functions();
