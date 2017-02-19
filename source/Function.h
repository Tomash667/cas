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
	bool have_def_value, pass_by_ref;

	ArgInfo(bool bvalue) : vartype(V_BOOL), bvalue(bvalue), have_def_value(true), pass_by_ref(false) {}
	ArgInfo(char cvalue) : vartype(V_CHAR), cvalue(cvalue), have_def_value(true), pass_by_ref(false) {}
	ArgInfo(int value) : vartype(V_INT), value(value), have_def_value(true), pass_by_ref(false) {}
	ArgInfo(float fvalue) : vartype(V_FLOAT), fvalue(fvalue), have_def_value(true), pass_by_ref(false) {}
	ArgInfo(VarType vartype, int value, bool have_def_value) : vartype(vartype), value(value), have_def_value(have_def_value), pass_by_ref(false) {}

	inline VarType GetDeclaredVarType() const
	{
		VarType v = vartype;
		if(pass_by_ref)
		{
			v.subtype = v.type;
			v.type = V_REF;
		}
		return v;
	}
};

// special function type
enum SpecialFunction
{
	SF_NO,
	SF_CTOR,
	SF_DTOR,
	SF_CAST,
	SF_ADDREF,
	SF_RELEASE
};

// common for parse & code function
struct CommonFunction
{
	enum FLAGS
	{
		F_THISCALL = 1 << 0,
		F_IMPLICIT = 1 << 1,
		F_BUILTIN = 1 << 2,
		F_DELETE = 1 << 3,
		F_CODE = 1 << 4,
		F_STATIC = 1 << 5,
		F_DEFAULT = 1 << 6
	};

#ifdef _DEBUG
	string decl;
#endif
	string name;
	VarType result;
	int index, type, flags;
	vector<ArgInfo> arg_infos;
	uint required_args;
	SpecialFunction special;

	bool Equal(CommonFunction& f) const;

	inline bool IsBuiltin() const { return IS_SET(flags, F_BUILTIN); }
	inline bool IsCode() const { return IS_SET(flags, F_CODE); }
	inline bool IsStatic() const { return IS_SET(flags, F_STATIC); }
	inline bool IsDefault() const { return IS_SET(flags, F_DEFAULT); }
	inline bool IsDeleted() const { return IS_SET(flags, F_DELETE); }
	inline bool IsImplicit() const { return IS_SET(flags, F_IMPLICIT); }

	// Should function pass this as first argument, 
	// Non static methods, except code ctors
	inline bool PassThis() const
	{
		
		return type != V_VOID && !(IsStatic() || (IsCode() && special == SF_CTOR));
	}
};

// code function
struct Function : CommonFunction
{
	void* clbk;
};

// script function
struct UserFunction
{
	string name;
	uint pos;
	uint locals;
	VarType result;
	vector<VarType> args;
	int type, index;
};
