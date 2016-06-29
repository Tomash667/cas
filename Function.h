#pragma once

#include "Type.h"

// function callback
typedef void(*VoidF)();
struct Function;

// function argument
struct ArgInfo
{
	int type;
	union
	{
		bool bvalue;
		int value;
		float fvalue;
	};
	bool have_def_value;

	ArgInfo(bool bvalue) : type(V_BOOL), bvalue(bvalue), have_def_value(true) {}
	ArgInfo(int value) : type(V_INT), value(value), have_def_value(true) {}
	ArgInfo(float fvalue) : type(V_FLOAT), fvalue(fvalue), have_def_value(true) {}
	ArgInfo(int type, int value, bool have_def_value) : type(type), value(value), have_def_value(have_def_value) {}
};

// common for parse & code function
struct CommonFunction
{
	string name;
	int result;
	vector<ArgInfo> arg_infos;
	uint required_args;
	bool method;
};

// code function
extern vector<Function*> functions;
struct Function : CommonFunction
{
	void* clbk;
	int index;
	int type;

	static inline Function* Find(const string& id)
	{
		for(Function* f : functions)
		{
			if(f->name == id && !f->type)
				return f;
		}
		return nullptr;
	}
};

// script function
struct UserFunction
{
	uint pos;
	uint locals;
	int result;
	vector<int> args;
};
