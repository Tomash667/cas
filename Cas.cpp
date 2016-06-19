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
	V_STRING, "string", true
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

	// parse
	ParseContext ctx;
	ctx.input = input;
	ctx.optimize = optimize;
	if(!Parse(ctx))
		return false;

	// decompile
	if(decompile)
		Decompile(ctx);

	// convert functions
	RunContext rctx;
	rctx.globals = ctx.globals;
	rctx.entry_point = ctx.entry_point;
	rctx.code = std::move(ctx.code);
	rctx.strs = std::move(ctx.strs);
	rctx.ufuncs.resize(ctx.ufuncs.size());
	for(uint i = 0; i < rctx.ufuncs.size(); ++i)
	{
		RunFunction& f = rctx.ufuncs[i];
		UserFunction& f2 = *ctx.ufuncs[i];
		f.locals = f2.locals;
		f.pos = f2.pos;
	}

	// run
	RunCode(rctx);
	return true;
}
