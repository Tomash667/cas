#pragma once

enum VAR_TYPE
{
	V_VOID,
	V_BOOL,
	V_INT,
	V_FLOAT,
	V_STRING,
	V_MAX
};

struct VarInfo
{
	VAR_TYPE type;
	cstring name;
	bool reg;
};

extern VarInfo var_info[V_MAX];
