#include "Pch.h"
#include "Base.h"
#include "Function.h"
#include "Run.h"
#define NULL nullptr
#include <conio.h>
#undef NULL

Function& AddFunction(cstring name, VoidF clbk)
{
	Function& f = Add1(functions);
	f.name = name;
	f.clbk = clbk;
	f.result = V_VOID;
	f.index = functions.size() - 1;
	return f;
}

void f_print()
{
	Var& v = stack.back();
	cout << v.str->s;
	v.str->Release();
	stack.pop_back();
}

void f_println()
{
	Var& v = stack.back();
	cout << v.str->s;
	cout << '\n';
	v.str->Release();
	stack.pop_back();
}

void f_getint()
{
	int val;
	cin >> val;
	stack.push_back(Var(val));
}

void f_getfloat()
{
	float val;
	cin >> val;
	stack.push_back(Var(val));
}

void f_getstr()
{
	Str* str = Str::Get();
	cin >> str->s;
	str->refs = 1;
	stack.push_back(Var(str));
}

void f_pause()
{
	_getch();
}

void RegisterFunctions()
{
	AddFunction("print", f_print).args.push_back(V_STRING);
	AddFunction("println", f_println).args.push_back(V_STRING);
	AddFunction("getint", f_getint).result = V_INT;
	AddFunction("getfloat", f_getfloat).result = V_FLOAT;
	AddFunction("getstr", f_getstr).result = V_STRING;
	AddFunction("pause", f_pause);
}
