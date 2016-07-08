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

// special function type
enum SpecialFunction
{
	SF_NO,
	SF_CTOR
};

// common for parse & code function
struct CommonFunction
{
	string name;
	int result, index;
	vector<ArgInfo> arg_infos;
	uint required_args;
	SpecialFunction special;
	bool method;

	bool Equal(CommonFunction& f) const;
	cstring GetName(uint var_offfset = 0) const;
};

// code function
extern vector<Function*> functions;
struct Function : CommonFunction
{
	void* clbk;
	int type;

	static Function* Find(const string& name);
	static Function* FindEqual(Function& fc);
};

// script function
struct UserFunction
{
	uint pos;
	uint locals;
	int result;
	vector<int> args;
};
