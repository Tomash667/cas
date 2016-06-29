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

struct INT2s
{
	int x, y;
};

int f_int2_sum(INT2s& i)
{
	return i.x + i.y;
}

INT2s f_create_int2(int x, int y)
{
	INT2s i;
	i.x = x;
	i.y = y;
	return i;
}

int f_sum_int2(INT2s& i)
{
	return i.x + i.y;
}

void AddParserType(Type* type);

void AddType(cstring type_name, int size, VAR_TYPE var_type, bool reg = true)
{
	Type* type = new Type;
	type->name = type_name;
	type->size = size;
	type->index = types.size();
	assert(type->index == (int)var_type);
	types.push_back(type);
	if(reg)
		AddParserType(type);
}

void InitCoreLib()
{
	// types
	AddType("void", 0, V_VOID);
	AddType("bool", sizeof(bool), V_BOOL);
	AddType("int", sizeof(int), V_INT);
	AddType("float", sizeof(float), V_FLOAT);
	AddType("string", sizeof(string), V_STRING);
	AddType("ref", 0, V_REF, false);
	AddType("special", 0, V_SPECIAL, false);

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

	// INT2
	cas::AddType("INT2", sizeof(INT2s));
	cas::AddMember("INT2", "int x", offsetof(INT2s, x));
	cas::AddMember("INT2", "int y", offsetof(INT2s, y));
	cas::AddMethod("INT2", "int sum()", f_int2_sum);
	cas::AddFunction("INT2 create_int2(int x, int y)", f_create_int2);
	cas::AddFunction("int sum_int2(INT2 i)", f_sum_int2);
}
