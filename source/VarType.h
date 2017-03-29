#pragma once

class IModuleProxy;

// Builtin core types
enum CoreVarType
{
	V_VOID,
	V_BOOL,
	V_CHAR,
	V_INT,
	V_FLOAT,
	V_STRING,
	V_REF,
	V_SPECIAL,
	V_TYPE,
	V_MAX
};

inline bool IsSimple(int type) { return type == V_BOOL || type == V_CHAR || type == V_INT || type == V_FLOAT; }

// Variable type
// Allows complex types (reference to type or enum subtype)
struct VarType
{
	int type, subtype;

	VarType() {}
	VarType(nullptr_t) : type(0), subtype(0) {}
	VarType(CoreVarType core) : type(core), subtype(0) {}
	VarType(int type, int subtype) : type(type), subtype(subtype) {}

	bool operator == (const VarType& vartype) const
	{
		return type == vartype.type && subtype == vartype.subtype;
	}

	bool operator != (const VarType& vartype) const
	{
		return type != vartype.type || subtype != vartype.subtype;
	}

	int GetType() const
	{
		if(type == V_REF)
			return subtype;
		else
			return type;
	}
};
