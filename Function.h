#pragma once

#include "Type.h"

// function callback
typedef void(*VoidF)();
struct Function;

// function argument
struct ArgInfo
{
	VarType type;
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
	ArgInfo(VarType type, int value, bool have_def_value) : type(type), value(value), have_def_value(have_def_value) {}
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
	VarType result;
	int index, type;
	vector<ArgInfo> arg_infos;
	uint required_args;
	SpecialFunction special;
	bool method;

	bool Equal(CommonFunction& f) const;
};

// code function
struct Function : CommonFunction
{
	void* clbk;
};

// script function
struct UserFunction
{
	uint pos;
	uint locals;
	VarType result;
	vector<VarType> args;
	int type;
};
