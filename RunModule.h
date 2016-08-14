#pragma once

#include "Function.h"

struct RunModule
{
	RunModule(Module* parent);
	~RunModule();

	Type* GetType(int index);
	Function* GetFunction(int index);

	Module* parent;
	vector<Str*> strs;
	vector<int> code;
	vector<UserFunction> ufuncs;
	vector<Type*> types;
	uint globals, entry_point;
	CoreVarType result;
};
