#include "Pch.h"
#include "Base.h"
#include "Run.h"
#include "Parse.h"
#include "Function.h"
#define NULL nullptr
#include <conio.h>
#undef NULL

cstring var_name[V_MAX] = {
	"void",
	"int",
	"string"
};

Logger* logger;

bool ParseAndRun(cstring input)
{
	RegisterFunctions();

	ParseContext ctx;
	ctx.input = input;

	if(!Parse(ctx))
	{
		_getch();
		return false;
	}

	RunCode(ctx.code, ctx.strs, ctx.vars);

	return true;
}
