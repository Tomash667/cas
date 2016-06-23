#pragma once

#include "Var.h"

struct Function;

struct Type
{
	string name;
	vector<Function*> funcs;
	VAR_TYPE builtin_type;

	Function* FindFunction(const string& name);
};

extern vector<Type*> types;

inline Type* FindType(cstring name)
{
	assert(name);
	for(Type* type : types)
	{
		if(type->name == name)
			return type;
	}
	return nullptr;
}

inline Type* FindType(VAR_TYPE builtin_type)
{
	for(Type* type : types)
	{
		if(type->builtin_type == builtin_type)
			return type;
	}
	return nullptr;
}
