#include "Pch.h"
#include "Base.h"
#include "Run.h"
#include "Parse.h"
#include "Function.h"

cstring var_name[V_MAX] = {
	"void",
	"int",
	"float",
	"string"
};

Logger* logger;
bool inited;

bool ParseAndRun(cstring input)
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

	if(!Parse(ctx))
		return false;

	RunCode(ctx.code, ctx.strs, ctx.vars);

	return true;
}
