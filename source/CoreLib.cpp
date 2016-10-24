#include "Pch.h"
#include "CasImpl.h"
#include "Type.h"
#include "Module.h"

// for _getch
#define NULL nullptr
#include <conio.h>
#undef NULL

static std::istream* s_input;
static std::ostream* s_output;
static bool s_use_getch;

void f_print(string& str)
{
	(*s_output) << str;
}

void f_println0()
{
	(*s_output) << '\n';
}

void f_println(string& str)
{
	(*s_output) << str;
	(*s_output) << '\n';
}

int f_getint()
{
	int val;
	(*s_input) >> val;
	return val;
}

float f_getfloat()
{
	float val;
	(*s_input) >> val;
	return val;
}

string f_getstr()
{
	string s;
	(*s_input) >> s;
	return s;
}

void f_pause()
{
	if(s_use_getch)
		_getch();
}

int f_string_length(string& str)
{
	return str.length();
}

int f_int_abs(int a)
{
	return abs(a);
}

float f_float_abs(float a)
{
	return abs(a);
}

void InitCoreLib(Module& module, std::istream* input, std::ostream* output, bool use_getch)
{
	assert(input && output);

	s_input = input;
	s_output = output;
	s_use_getch = use_getch;

	// types
	module.AddCoreType("void", 0, V_VOID, false);
	module.AddCoreType("bool", sizeof(bool), V_BOOL, false);
	module.AddCoreType("int", sizeof(int), V_INT, false);
	module.AddCoreType("float", sizeof(float), V_FLOAT, false);
	module.AddCoreType("string", sizeof(string), V_STRING, true);
	module.AddCoreType("ref", 0, V_REF, true, true);
	module.AddCoreType("special", 0, V_SPECIAL, false, true);

	// type functions
	module.AddMethod("string", "int length()", f_string_length);
	module.AddMethod("int", "int abs()", f_int_abs);
	module.AddMethod("float", "float abs()", f_float_abs);

	// functions
	module.AddFunction("void print(string str)", f_print);
	module.AddFunction("void println()", f_println0);
	module.AddFunction("void println(string str)", f_println);
	module.AddFunction("int getint()", f_getint);
	module.AddFunction("float getfloat()", f_getfloat);
	module.AddFunction("string getstr()", f_getstr);
	module.AddFunction("void pause()", f_pause);
}
