#pragma once

#include "Function.h"

struct RunContext
{
	vector<int> code;
	vector<Str*> strs;
	vector<UserFunction> ufuncs;
	uint globals, entry_point;
};

void RunCode(RunContext& ctx);
