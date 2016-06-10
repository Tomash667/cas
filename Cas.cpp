#include "Pch.h"
#include "Base.h"
#include "Run.h"
#include "Parse.h"
#include "Function.h"

cstring var_name[V_MAX] = {
	"void",
	"bool",
	"int",
	"float",
	"string"
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
