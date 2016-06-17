#include "Pch.h"
#include "Base.h"
#include "Run.h"
#include "Parse.h"
#include "Function.h"

VarInfo var_info[V_MAX] = {
	V_VOID, "void", true,
	V_BOOL, "bool", true,
	V_INT, "int", true,
	V_FLOAT, "float", true,
	V_STRING, "string", true,
	V_REF, "ref", false
};

Logger* logger;
static bool inited;

bool ParseAndRun(cstring input, bool optimize, bool decompile)
{
	if(!inited)
	{
		RegisterFunctions();
		InitializeParser();
		inited = true;
	}
	else
		CleanupParser();

	ParseContext ctx;
	ctx.input = input;
	ctx.optimize = optimize;

	if(!Parse(ctx))
		return false;

	if(decompile)
		Decompile(ctx);

	RunCode(ctx.code, ctx.strs, ctx.vars);

	return true;
}
