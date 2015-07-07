#include "Base.h"
#include "Function.h"
#include "Stack.h"
#include <iostream>
#include <conio.h>

vector<FunctionInfo> functions;

void f_print()
{
	printf(stack.back().v.str->s.c_str());
	stack.back().Clean();
	stack.pop_back();
}

void f_getint()
{
	int a;
	scanf_s("%d", &a);
	stack.push_back(Var(a));
}

void f_getfloat()
{
	float a;
	scanf_s("%f", &a);
	stack.push_back(Var(a));
}

void f_getstring()
{
	Str* s = StrPool.Get();
	std::getline(std::cin, s->s);
	s->refs = 1;
	stack.push_back(Var(s));
}

void f_pause()
{
	_getch();
}

void f_random()
{
	int right = stack.back().v.i;
	stack.pop_back();
	int left = stack.back().v.i;
	if(left == right)
		return;
	if(left > right)
		std::swap(left, right);
	stack.back().v.i = rand() % (right - left + 1) + left;
}

void register_functions()
{
	{
		FunctionInfo& f = Add1(functions);
		f.name = "print";
		f.ptr = f_print;
		f.result = VOID;
		f.params[0] = STR;
		f.params_count = 1;
	}
	{
		FunctionInfo& f = Add1(functions);
		f.name = "getint";
		f.ptr = f_getint;
		f.result = INT;
		f.params_count = 0;
	}
	{
		FunctionInfo& f = Add1(functions);
		f.name = "getfloat";
		f.ptr = f_getfloat;
		f.result = FLOAT;
		f.params_count = 0;
	}
	{
		FunctionInfo& f = Add1(functions);
		f.name = "getstr";
		f.ptr = f_getstring;
		f.result = STR;
		f.params_count = 0;
	}
	{
		FunctionInfo& f = Add1(functions);
		f.name = "pause";
		f.ptr = f_pause;
		f.result = VOID;
		f.params_count = 0;
	}
	{
		FunctionInfo& f = Add1(functions);
		f.name = "random";
		f.ptr = f_random;
		f.result = INT;
		f.params[0] = INT;
		f.params[1] = INT;
		f.params_count = 2;
	}
}
