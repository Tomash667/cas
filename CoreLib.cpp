#include "Pch.h"
#include "Base.h"
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

struct INT2s
{
	int x, y;
};

int f_int2_sum(INT2s& i)
{
	return i.x + i.y;
}

int f_int2_sum3(INT2s& i, int a)
{
	return i.x + i.y + a;
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

struct Vec2
{
	float x, y;
};

Vec2 f_create_vec2(float x, float y)
{
	Vec2 v;
	v.x = x;
	v.y = y;
	return v;
}

void f_wypisz_vec2(Vec2& v)
{
	(*s_output) << "x:" << v.x << " y:" << v.y << "\n";
}

struct Vec3
{
	float x, y, z;
};

Vec3 f_create_vec3(float x, float y, float z)
{
	Vec3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

void f_wypisz_vec3(Vec3& v)
{
	(*s_output) << "x:" << v.x << " y:" << v.y << " z:" << v.z << "\n";
}

INT2 f_int2c_ctor0()
{
	return INT2(0, 0);
}

INT2 f_int2c_ctor1(int xy)
{
	return INT2(xy);
}

INT2 f_int2c_ctor2(int x, int y)
{
	return INT2(x, y);
}

void f_wypisz_int2c(INT2& i)
{
	(*s_output) << "x:" << i.x << " y:" << i.y << "\n";
}

void InitCoreLib(Module* module, std::istream* input, std::ostream* output, bool use_getch)
{
	assert(module && input && output);

	s_input = input;
	s_output = output;
	s_use_getch = use_getch;

	// types
	module->AddCoreType("void", 0, V_VOID, false);
	module->AddCoreType("bool", sizeof(bool), V_BOOL, false);
	module->AddCoreType("int", sizeof(int), V_INT, false);
	module->AddCoreType("float", sizeof(float), V_FLOAT, false);
	module->AddCoreType("string", sizeof(string), V_STRING, true);
	module->AddCoreType("ref", 0, V_REF, true, true);
	module->AddCoreType("special", 0, V_SPECIAL, false, true);

	// type functions
	module->AddMethod("string", "int length()", f_string_length);
	module->AddMethod("int", "int abs()", f_int_abs);
	module->AddMethod("float", "float abs()", f_float_abs);

	// functions
	module->AddFunction("void print(string str)", f_print);
	module->AddFunction("void println()", f_println0);
	module->AddFunction("void println(string str)", f_println);
	module->AddFunction("int getint()", f_getint);
	module->AddFunction("float getfloat()", f_getfloat);
	module->AddFunction("string getstr()", f_getstr);
	module->AddFunction("void pause()", f_pause);

	// INT2
	module->AddType<INT2s>("INT2");
	module->AddMember("INT2", "int x", offsetof(INT2s, x));
	module->AddMember("INT2", "int y", offsetof(INT2s, y));
	module->AddMethod("INT2", "int sum()", f_int2_sum);
	module->AddMethod("INT2", "int sum(int a)", f_int2_sum3);
	module->AddFunction("INT2 create_int2(int x, int y)", f_create_int2);
	module->AddFunction("int sum_int2(INT2 i)", f_sum_int2);

	// Vec2
	module->AddType<Vec2>("Vec2");
	module->AddMember("Vec2", "float x", offsetof(Vec2, x));
	module->AddMember("Vec2", "float y", offsetof(Vec2, y));
	module->AddFunction("Vec2 create_vec2(float x, float y)", f_create_vec2);
	module->AddFunction("void wypisz_vec2(Vec2 v)", f_wypisz_vec2);

	// Vec3 (pod > 8 byte)
	module->AddType<Vec3>("Vec3");
	module->AddMember("Vec3", "float x", offsetof(Vec3, x));
	module->AddMember("Vec3", "float y", offsetof(Vec3, y));
	module->AddMember("Vec3", "float z", offsetof(Vec3, z));
	module->AddFunction("Vec3 create_vec3(float x, float y, float z)", f_create_vec3);
	module->AddFunction("void wypisz_vec3(Vec3 v)", f_wypisz_vec3);

	// INT2 have ctor
	module->AddType<INT2>("INT2c");
	module->AddMember("INT2c", "int x", offsetof(INT2, x));
	module->AddMember("INT2c", "int y", offsetof(INT2, y));
	module->AddMethod("INT2c", "INT2c()", f_int2c_ctor0);
	module->AddMethod("INT2c", "INT2c(int xy)", f_int2c_ctor1);
	module->AddMethod("INT2c", "INT2c(int x, int y)", f_int2c_ctor2);
	module->AddFunction("void wypisz_int2c(INT2c i)", f_wypisz_int2c);
}
