#pragma once

#include "Var.h"
#include "Type.h"

typedef void(*VoidF)();

struct CommonFunction
{
	string name;
	VAR_TYPE result;
	vector<ArgInfo> arg_infos;
	uint required_args;
	bool method;
};

struct Function : CommonFunction
{
	void* clbk;
	int index;
	Type* type;
};

struct UserFunction
{
	uint pos;
	uint locals;
#ifdef _DEBUG
	vector<VAR_TYPE> args;
#else
	uint args;
#endif
	VAR_TYPE result;

	inline uint GetArgs() const
	{
#ifdef _DEBUG
		return args.size();
#else
		return args;
#endif
	}
};

extern vector<Function*> functions;

inline Function* FindFunction(const string& id, VAR_TYPE var_type = V_VOID)
{
	for(Function* f : functions)
	{
		if(id == f->name)
		{
			if(var_type == V_VOID)
			{
				if(!f->type)
					return f;
			}
			else
			{
				if(f->type->builtin_type == var_type)
					return f;
			}
		}
	}
	return nullptr;
}
