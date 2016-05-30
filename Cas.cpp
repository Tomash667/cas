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

int main()
{
	RegisterFunctions();

	ParseContext ctx;
	ctx.input = "print(\"Podaj a: \"); int a; a = getint(); print(\"Odwrocone a: \"); print(-a); pause();";

	if(!Parse(ctx))
	{
		_getch();
		return 1;
	}

	RunCode(ctx.code, ctx.strs, ctx.vars);

	return 0;
}
