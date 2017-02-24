#pragma once

#include "Class.h"
#include "Type.h"

struct Class;
struct RefVar;
struct Str;

// Call context variable
struct Var
{
	VarType vartype;
	union
	{
		bool bvalue;
		char cvalue;
		int value;
		float fvalue;
		Str* str;
		RefVar* ref;
		Class* clas;
	};

	explicit Var() : vartype(V_VOID) {}
	explicit Var(CoreVarType type) : vartype(type) {}
	explicit Var(bool bvalue) : vartype(V_BOOL), bvalue(bvalue) {}
	explicit Var(char cvalue) : vartype(V_CHAR), cvalue(cvalue) {}
	explicit Var(int value) : vartype(V_INT), value(value) {}
	explicit Var(float fvalue) : vartype(V_FLOAT), fvalue(fvalue) {}
	explicit Var(Str* str) : vartype(V_STRING), str(str) {}
	Var(RefVar* ref, int subtype) : vartype(VarType(V_REF, subtype)), ref(ref) {}
	explicit Var(Class* clas) : vartype(clas->type->index, 0), clas(clas) {}
	Var(VarType vartype, int value) : vartype(vartype), value(value) {}
};
