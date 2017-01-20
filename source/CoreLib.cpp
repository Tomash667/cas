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

static bool f_string_empty(string& str)
{
	return str.empty();
}

static void f_string_clear(string& str)
{
	str.clear();
}

static int f_int_abs(int a)
{
	return abs(a);
}

static int f_int_parse(string& s)
{
	int i;
	if(!TextHelper::ToInt(s.c_str(), i))
		throw CasException(Format("Invalid int format '%s'.", s.c_str()));
	return i;
}

static bool f_int_try_parse(string& s, int& i)
{
	return TextHelper::ToInt(s.c_str(), i);
}

static float f_float_parse(string& s)
{
	float f;
	if(!TextHelper::ToFloat(s.c_str(), f))
		throw CasException(Format("Invalid float format '%s'.", s.c_str()));
	return f;
}

static bool f_float_try_parse(string& s, float& f)
{
	return TextHelper::ToFloat(s.c_str(), f);
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

static bool f_bool_parse(string& s)
{
	bool b;
	if(!TextHelper::ToBool(s.c_str(), b))
		throw CasException(Format("Invalid bool format '%s'.", s.c_str()));
	return b;
}

static bool f_bool_try_parse(string& s, bool& b)
{
	return TextHelper::ToBool(s.c_str(), b);
}

static bool f_char_try_parse(string& s, char& c)
{
	if(s.length() != 1u)
		return false;
	c = s[0];
	return true;
}

static char f_char_parse(string& s)
{
	char c;
	if(!f_char_try_parse(s, c))
		throw CasException(Format("Invalid char format '%s'.", s.c_str()));
	return c;
}

static void InitCoreLib(Module& module, Settings& settings)
{
	assert(settings.input && settings.output);

	s_input = (std::istream*)settings.input;
	s_output = (std::ostream*)settings.output;
	s_use_getch = settings.use_getch;

	// types
	Type* _void = module.AddCoreType("void", 0, V_VOID, 0);
	Type* _bool = module.AddCoreType("bool", sizeof(bool), V_BOOL, 0);
	Type* _char = module.AddCoreType("char", sizeof(char), V_CHAR, 0);
	Type* _int = module.AddCoreType("int", sizeof(int), V_INT, 0);
	Type* _float = module.AddCoreType("float", sizeof(float), V_FLOAT, 0);
	Type* _string = module.AddCoreType("string", sizeof(string), V_STRING, Type::PassByValue);
	Type* _array = module.AddCoreType("array", 0, V_ARRAY, Type::Ref | Type::Hidden | Type::Generic | Type::Class);
	_array->generic_param = "T";
	module.AddCoreType("ref", 0, V_REF, Type::Ref | Type::Hidden);
	module.AddCoreType("special", 0, V_SPECIAL, Type::Hidden);
	module.AddCoreType("type", 0, V_TYPE, Type::Hidden);
	module.AddCoreType("complex", 0, V_COMPLEX, Type::Hidden);
	module.AddCoreType("generic", 0, V_GENERIC, Type::Hidden);
	// bool methods
	module.AddMethod(_bool, "bool operator = (bool b)", nullptr);
	module.AddMethod(_bool, "static bool Parse(string& s)", f_bool_parse);
	module.AddMethod(_bool, "static bool TryParse(string& s, bool& b)", f_bool_try_parse);
	// char methods
	module.AddMethod(_char, "char operator = (char c)", nullptr);
	module.AddMethod(_char, "static char Parse(string& s)", f_char_parse);
	module.AddMethod(_char, "static bool TryParse(string& s, char& c)", f_char_try_parse);
	// int methods
	module.AddMethod(_int, "int operator = (int i)", nullptr);
	module.AddMethod(_int, "int abs()", f_int_abs);
	module.AddMethod(_int, "static int Parse(string& s)", f_int_parse);
	module.AddMethod(_int, "static bool TryParse(string& s, int& i)", f_int_try_parse);
	// float methods
	module.AddMethod(_float, "float operator = (float f)", nullptr);
	module.AddMethod(_float, "float abs()", f_float_abs);
	module.AddMethod(_float, "static float Parse(string& s)", f_float_parse);
	module.AddMethod(_float, "static bool TryParse(string& s, float& f)", f_float_try_parse);
	// string methods
	module.AddMethod(_string, "string operator = (string& s)", nullptr);
	module.AddMethod(_string, "int length()", f_string_length);
	module.AddMethod(_string, "bool empty()", f_string_empty);
	module.AddMethod(_string, "void clear()", f_string_clear);
	// array methods
	module.AddMethod(_array, "void add(T& item)", &Array::add);
	module.AddMethod(_array, "T& at(int index)", &Array::at);
	module.AddMethod(_array, "T& back(int offset=0)", &Array::back);
	module.AddMethod(_array, "void clear()", &Array::clear);
	module.AddMethod(_array, "int count()", &Array::count);
	module.AddMethod(_array, "bool empty()", &Array::empty);
	module.AddMethod(_array, "T& front(int offset=0)", &Array::front);
	module.AddMethod(_array, "void insert(int index, T& item)", &Array::insert);
	module.AddMethod(_array, "void pop()", &Array::pop);
	module.AddMethod(_array, "void remove(int index)", &Array::remove);
	module.AddMethod(_array, "void resize(int size)", &Array::resize);
	module.AddMethod(_array, "T& operator [](int index)", &Array::operator []);
	module.AddMethod(_array, "bool operator == (T[] arr)", &Array::operator ==);
	module.AddMethod(_array, "bool operator != (T[] arr)", &Array::operator !=);
	module.AddMethod(_array, "void operator += (T& item)", AsMethod(Array, operator +=, void, (int*)));
	module.AddMethod(_array, "void operator += (T[] arr)", AsMethod(Array, operator +=, void, (Array&)));
	// functions
	module.AddFunction("void print(string& str)", f_print);
	module.AddFunction("void println()", f_println0);
	module.AddFunction("void println(string& str)", f_println);
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
	module.AddFunction("void Assert_AreEqual(string& expected, string& actual)", AsFunction(Assert_AreEqual, void, (string&, string&)));
	module.AddFunction("void Assert_AreNotEqual(string& not_expected, string& actual)", AsFunction(Assert_AreNotEqual, void, (string&, string&)));
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
