#include "Pch.h"
#include "Base.h"
#include "Type.h"
#include "Cas.h"

// for _getch
#define NULL nullptr
#include <conio.h>
#undef NULL

void f_print(string& str)
{
	cout << str;
}

void f_println(string& str)
{
	cout << str;
	cout << '\n';
}

int f_getint()
{
	int val;
	cin >> val;
	return val;
}

float f_getfloat()
{
	float val;
	cin >> val;
	return val;
}

string f_getstr()
{
	string s;
	cin >> s;
	return s;
}

void f_pause()
{
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

void AddType(cstring type_name, VAR_TYPE builtin_type)
{
	Type* type = new Type;
	type->name = type_name;
	type->builtin_type = builtin_type;
	types.push_back(type);
}

void InitCoreLib()
{
	// types
	AddType("bool", V_BOOL);
	AddType("int", V_INT);
	AddType("float", V_FLOAT);
	AddType("string", V_STRING);

	// type functions
	cas::AddMethod("string", "int length()", f_string_length);
	cas::AddMethod("int", "int abs()", f_int_abs);
	cas::AddMethod("float", "float abs()", f_float_abs);

	// functions
	cas::AddFunction("void print(string str)", f_print);
	cas::AddFunction("void println(string str)", f_println);
	cas::AddFunction("int getint()", f_getint);
	cas::AddFunction("float getfloat()", f_getfloat);
	cas::AddFunction("string getstr()", f_getstr);
	cas::AddFunction("void pause()", f_pause);
}
