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

static char f_getchar()
{
	char val;
	(*s_input) >> val;
	return val;
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
	module.AddCoreType("char", sizeof(char), V_CHAR, false);
	module.AddCoreType("int", sizeof(int), V_INT, false);
	module.AddCoreType("float", sizeof(float), V_FLOAT, false);
	module.AddCoreType("string", sizeof(string), V_STRING, true);
	module.AddCoreType("ref", 0, V_REF, true, true);
	module.AddCoreType("special", 0, V_SPECIAL, false, true);
	module.AddCoreType("type", 0, V_TYPE, false, true);
	// bool methods
	module.AddMethod("bool", "implicit char operator cast()", nullptr);
	module.AddMethod("bool", "implicit int operator cast()", nullptr);
	module.AddMethod("bool", "implicit float operator cast()", nullptr);
	module.AddMethod("bool", "implicit string operator cast()", nullptr);
	module.AddMethod("bool", "bool operator = (bool b)", nullptr);
	// char methods
	module.AddMethod("char", "implicit bool operator cast()", nullptr);
	module.AddMethod("char", "implicit int operator cast()", nullptr);
	module.AddMethod("char", "implicit float operator cast()", nullptr);
	module.AddMethod("char", "implicit string operator cast()", nullptr);
	module.AddMethod("char", "char operator = (char c)", nullptr);
	// int methods
	module.AddMethod("int", "implicit bool operator cast()", nullptr);
	module.AddMethod("int", "implicit char operator cast()", nullptr);
	module.AddMethod("int", "implicit float operator cast()", nullptr);
	module.AddMethod("int", "implicit string operator cast()", nullptr);
	module.AddMethod("int", "int operator = (int i)", nullptr);
	module.AddMethod("int", "int abs()", f_int_abs);
	// float methods
	module.AddMethod("float", "implicit bool operator cast()", nullptr);
	module.AddMethod("float", "implicit char operator cast()", nullptr);
	module.AddMethod("float", "implicit int operator cast()", nullptr);
	module.AddMethod("float", "implicit string operator cast()", nullptr);
	module.AddMethod("float", "float operator = (float f)", nullptr);
	module.AddMethod("float", "float abs()", f_float_abs);
	// string methods
	module.AddMethod("float", "string operator = (string s)", nullptr);
	module.AddMethod("string", "int length()", f_string_length);
	
	// functions
	module.AddFunction("void print(string str)", f_print);
	module.AddFunction("void println()", f_println0);
	module.AddFunction("void println(string str)", f_println);
	module.AddFunction("int getint()", f_getint);
	module.AddFunction("float getfloat()", f_getfloat);
	module.AddFunction("string getstr()", f_getstr);
	module.AddFunction("void pause()", f_pause);
	module.AddFunction("char getchar()", f_getchar);
}

static void AddAssert(cstring msg)
{
	auto loc = cas::GetCurrentLocation();
	cstring formated;
	if(loc.second != -1)
		formated = Format("Function: %s(%d). Message: %s", loc.first, loc.second, msg);
	else
		formated = Format("Function: %s. Message: %s", loc.first, msg);
	asserts.push_back(formated);
}

static void Assert_AreEqual(bool expected, bool actual)
{
	if(expected != actual)
		AddAssert(Format("Expected <%s>, actual <%s>.", (expected ? "true" : "false"), (actual ? "true" : "false")));
}

static void Assert_AreNotEqual(bool not_expected, bool actual)
{
	if(not_expected == actual)
		AddAssert(Format("Not expected <%s>, actual <%s>.", (not_expected ? "true" : "false"), (actual ? "true" : "false")));
}

static void Assert_AreEqual(char expected, char actual)
{
	if(expected != actual)
		AddAssert(Format("Expected <%s>, actual <%s>.", EscapeChar(expected), EscapeChar(actual)));
}

static void Assert_AreNotEqual(char not_expected, char actual)
{
	if(not_expected == actual)
		AddAssert(Format("Not expected <%s>, actual <%s>.", EscapeChar(not_expected), EscapeChar(actual)));
}

static void Assert_AreEqual(int expected, int actual)
{
	if(expected != actual)
		AddAssert(Format("Expected <%d>, actual <%d>.", expected, actual));
}

static void Assert_AreNotEqual(int not_expected, int actual)
{
	if(not_expected == actual)
		AddAssert(Format("Not expected <%d>, actual <%d>.", not_expected, actual));
}

static void Assert_AreEqual(float expected, float actual)
{
	if(expected != actual)
		AddAssert(Format("Expected <%.2g>, actual <%.2g>.", expected, actual));
}

static void Assert_AreNotEqual(float not_expected, float actual)
{
	if(not_expected == actual)
		AddAssert(Format("Not expected <%.2g>, actual <%.2g>.", not_expected, actual));
}

static void Assert_AreEqual(string& expected, string& actual)
{
	if(expected != actual)
		AddAssert(Format("Expected <%s>, actual <%s>.", Escape(expected), Escape(actual)));
}

static void Assert_AreNotEqual(string& not_expected, string& actual)
{
	if(not_expected == actual)
		AddAssert(Format("Not expected <%s>, actual <%s>.", Escape(not_expected), Escape(actual)));
}

static void Assert_IsTrue(bool value)
{
	if(!value)
		AddAssert("True expected.");
}

static void Assert_IsFalse(bool value)
{
	if(value)
		AddAssert("False expected.");
}

static void Assert_Break()
{
	DebugBreak();
}

vector<string>& cas::GetAsserts()
{
	return asserts;
}

static void InitDebugLib(Module& module)
{
	module.AddFunction("void Assert_AreEqual(bool expected, bool actual)", AsFunction(Assert_AreEqual, void, (bool, bool)));
	module.AddFunction("void Assert_AreNotEqual(bool not_expected, bool actual)", AsFunction(Assert_AreNotEqual, void, (bool, bool)));
	module.AddFunction("void Assert_AreEqual(char expected, char actual)", AsFunction(Assert_AreEqual, void, (char, char)));
	module.AddFunction("void Assert_AreNotEqual(char not_expected, char actual)", AsFunction(Assert_AreNotEqual, void, (char, char)));
	module.AddFunction("void Assert_AreEqual(int expected, int actual)", AsFunction(Assert_AreEqual, void, (int, int)));
	module.AddFunction("void Assert_AreNotEqual(int not_expected, int actual)", AsFunction(Assert_AreNotEqual, void, (int, int)));
	module.AddFunction("void Assert_AreEqual(float expected, float actual)", AsFunction(Assert_AreEqual, void, (float, float)));
	module.AddFunction("void Assert_AreNotEqual(float not_expected, float actual)", AsFunction(Assert_AreNotEqual, void, (float, float)));
	module.AddFunction("void Assert_AreEqual(string expected, string actual)", AsFunction(Assert_AreEqual, void, (string&, string&)));
	module.AddFunction("void Assert_AreNotEqual(string not_expected, string actual)", AsFunction(Assert_AreNotEqual, void, (string&, string&)));
	module.AddFunction("void Assert_IsTrue(bool value)", Assert_IsTrue);
	module.AddFunction("void Assert_IsFalse(bool value)", Assert_IsFalse);
	module.AddFunction("void Assert_Break()", Assert_Break);
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
