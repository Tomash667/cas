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

void f_println0()
{
	cout << '\n';
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
	cout << "x:" << v.x << " y:" << v.y << "\n";
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
	cout << "x:" << v.x << " y:" << v.y << " z:" << v.z << "\n";
}

INT2 f_create_int2c(int x, int y)
{
	return INT2(x, y);
}

void f_wypisz_int2c(INT2& i)
{
	cout << "x:" << i.x << " y:" << i.y << "\n";
}

void AddParserType(Type* type);

void AddType(cstring type_name, int size, VAR_TYPE var_type, bool reg = true)
{
	Type* type = new Type;
	type->name = type_name;
	type->size = size;
	type->index = types.size();
	type->pod = true;
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
	cas::AddFunction("void println()", f_println0);
	cas::AddFunction("void println(string str)", f_println);
	cas::AddFunction("int getint()", f_getint);
	cas::AddFunction("float getfloat()", f_getfloat);
	cas::AddFunction("string getstr()", f_getstr);
	cas::AddFunction("void pause()", f_pause);

	// INT2
	cas::AddType<INT2s>("INT2");
	cas::AddMember("INT2", "int x", offsetof(INT2s, x));
	cas::AddMember("INT2", "int y", offsetof(INT2s, y));
	cas::AddMethod("INT2", "int sum()", f_int2_sum);
	cas::AddMethod("INT2", "int sum(int a)", f_int2_sum3);
	cas::AddFunction("INT2 create_int2(int x, int y)", f_create_int2);
	cas::AddFunction("int sum_int2(INT2 i)", f_sum_int2);

	// Vec2
	cas::AddType<Vec2>("Vec2");
	cas::AddMember("Vec2", "float x", offsetof(Vec2, x));
	cas::AddMember("Vec2", "float y", offsetof(Vec2, y));
	cas::AddFunction("Vec2 create_vec2(float x, float y)", f_create_vec2);
	cas::AddFunction("void wypisz_vec2(Vec2 v)", f_wypisz_vec2);

	// Vec3 (pod > 8 byte)
	cas::AddType<Vec3>("Vec3");
	cas::AddMember("Vec3", "float x", offsetof(Vec3, x));
	cas::AddMember("Vec3", "float y", offsetof(Vec3, y));
	cas::AddMember("Vec3", "float z", offsetof(Vec3, z));
	cas::AddFunction("Vec3 create_vec3(float x, float y, float z)", f_create_vec3);
	cas::AddFunction("void wypisz_vec3(Vec3 v)", f_wypisz_vec3);

	// INT2 have ctor
	cas::AddType<INT2>("INT2c");
	cas::AddMember("INT2c", "int x", offsetof(INT2, x));
	cas::AddMember("INT2c", "int y", offsetof(INT2, y));
	cas::AddFunction("INT2c create_int2c(int x, int y)", f_create_int2c);
	cas::AddFunction("void wypisz_int2c(INT2c i)", f_wypisz_int2c);
}
