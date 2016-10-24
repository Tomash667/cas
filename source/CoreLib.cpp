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
static vector<string> asserts;

static void f_print(string& str)
{
	(*s_output) << str;
}

static void f_println0()
{
	(*s_output) << '\n';
}

static void f_println(string& str)
{
	(*s_output) << str;
	(*s_output) << '\n';
}

static int f_getint()
{
	int val;
	(*s_input) >> val;
	return val;
}

static float f_getfloat()
{
	float val;
	(*s_input) >> val;
	return val;
}

static string f_getstr()
{
	string s;
	(*s_input) >> s;
	return s;
}

static void f_pause()
{
	if(s_use_getch)
		_getch();
}

static int f_string_length(string& str)
{
	return str.length();
}

static int f_int_abs(int a)
{
	return abs(a);
}

static float f_float_abs(float a)
{
	return abs(a);
}

static void InitCoreLib(Module& module, Settings& settings)
{
	assert(settings.input && settings.output);

	s_input = (std::istream*)settings.input;
	s_output = (std::ostream*)settings.output;
	s_use_getch = settings.use_getch;

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

static void Assert_AreEqual(int expected, int actual)
{
	if(expected != actual)
		asserts.push_back(Format("Expected <%d>, actual <%d>.", expected, actual));
}

static void Assert_AreNotEqual(int not_expected, int actual)
{
	if(not_expected == actual)
		asserts.push_back(Format("Not expected <%d>, actual <%d>.", not_expected, actual));
}

static void Assert_IsTrue(bool value)
{
	if(!value)
		asserts.push_back("True expected.");
}

static void Assert_IsFalse(bool value)
{
	if(value)
		asserts.push_back("False expected.");
}

vector<string>& cas::GetAsserts()
{
	return asserts;
}

static void InitDebugLib(Module& module)
{
	module.AddFunction("void Assert_AreEqual(int expected, int actual)", Assert_AreEqual);
	module.AddFunction("void Assert_AreNotEqual(int not_expected, int actual)", Assert_AreNotEqual);
	module.AddFunction("void Assert_IsTrue(bool value)", Assert_IsTrue);
	module.AddFunction("void Assert_IsFalse(bool value)", Assert_IsFalse);
}

void InitLib(Module& module, Settings& settings)
{
	if(settings.use_corelib)
	{
		InitCoreLib(module, settings);

		if(settings.use_debuglib)
			InitDebugLib(module);
	}
}
