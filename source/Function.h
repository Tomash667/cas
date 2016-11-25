#pragma once

#include "Type.h"

// function argument
struct ArgInfo
{
	VarType vartype;
	union
	{
		bool bvalue;
		char cvalue;
		int value;
		float fvalue;
	};
	bool have_def_value;

	ArgInfo(bool bvalue) : vartype(V_BOOL), bvalue(bvalue), have_def_value(true) {}
	ArgInfo(char cvalue) : vartype(V_CHAR), cvalue(cvalue), have_def_value(true) {}
	ArgInfo(int value) : vartype(V_INT), value(value), have_def_value(true) {}
	ArgInfo(float fvalue) : vartype(V_FLOAT), fvalue(fvalue), have_def_value(true) {}
	ArgInfo(VarType vartype, int value, bool have_def_value) : vartype(vartype), value(value), have_def_value(have_def_value) {}
};

// special function type
enum SpecialFunction
{
	SF_NO,
	SF_CTOR,
	SF_CAST
	//SF_ADDREF,
	//SF_RELEASE
};

// common for parse & code function
struct CommonFunction
{
	enum FLAGS
	{
		F_THISCALL = 1 << 0,
		F_IMPLICIT = 1 << 1,
		F_BUILTIN = 1 << 2
	};

	string name;
	VarType result;
	int index, type, flags;
	vector<ArgInfo> arg_infos;
	uint required_args;
	SpecialFunction special;

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
