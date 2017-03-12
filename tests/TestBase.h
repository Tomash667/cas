#pragma once

using namespace cas;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			template<>
			static wstring ToString(const GenericType& generic_type)
			{
				switch(generic_type)
				{
				case GenericType::Void:
					return L"void";
				case GenericType::Bool:
					return L"bool";
				case GenericType::Char:
					return L"char";
				case GenericType::Int:
					return L"int";
				case GenericType::Float:
					return L"float";
				case GenericType::String:
					return L"string";
				case GenericType::Class:
					return L"class";
				case GenericType::Struct:
					return L"struct";
				case GenericType::Enum:
					return L"enum";
				case GenericType::Invalid:
				default:
					return L"invalid";
				}
			}

			template<>
			static wstring ToString(const IModule::ParseResult& result)
			{
				switch(result)
				{
				case IModule::ValidationError:
					return L"ValidationError";
				case IModule::ParsingError:
					return L"ParsingError";
				case IModule::Ok:
					return L"Ok";
				default:
					return L"Invalid";
				}
			}
		}
	}
}


const bool CI_MODE = ((_CI_MODE - 1) == 0);

extern IEngine* engine;
extern IModule* current_module;
extern ICallContext* current_call_context;

struct TestSettings
{
	IModule* module;
	cstring filename;
	cstring code;
	cstring input;
	cstring output;
	cstring error;
	bool optimize;
};

void RunTest(const TestSettings& settings);

inline void RunFileTest(cstring filename, cstring input = "", cstring output = "", bool optimize = true)
{
	TestSettings s;
	s.module = current_module;
	s.filename = filename;
	s.code = nullptr;
	s.input = input;
	s.output = output;
	s.error = nullptr;
	s.optimize = optimize;
	RunTest(s);
}

inline void RunTest(cstring code, cstring input = "", cstring output = nullptr)
{
	TestSettings s;
	s.module = current_module;
	s.filename = nullptr;
	s.code = code;
	s.input = input;
	s.output = output;
	s.error = nullptr;
	s.optimize = true;
	RunTest(s);
}

inline void RunFailureTest(cstring code, cstring error, cstring input = "")
{
	TestSettings s;
	s.module = current_module;
	s.filename = nullptr;
	s.code = code;
	s.input = input;
	s.output = nullptr;
	s.error = error;
	s.optimize = true;
	RunTest(s);
}

inline void RunParsedTest(cstring input = "", cstring output = nullptr)
{
	TestSettings s;
	s.module = current_module;
	s.filename = nullptr;
	s.code = nullptr;
	s.input = input;
	s.output = output;
	s.error = nullptr;
	s.optimize = true;
	RunTest(s);
}

void CleanupErrors();
void CleanupAsserts();
void AssertError(cstring error);
void SetDecompile(bool decompile);
void SetResetParser(bool reset_parser);
void WriteOutput(cstring msg);

struct Retval
{
	ICallContext* call_context;
	static Retval* current;
	
	void IsVoid()
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Void, ret.type->GetGenericType());
	}

	void IsBool(bool expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Bool, ret.type->GetGenericType());
		Assert::AreEqual(expected, ret.bool_value);
	}

	void IsChar(char expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Char, ret.type->GetGenericType());
		Assert::AreEqual(expected, ret.char_value);
	}

	void IsInt(int expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Int, ret.type->GetGenericType());
		Assert::AreEqual(expected, ret.int_value);
	}

	void IsFloat(float expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Float, ret.type->GetGenericType());
		Assert::AreEqual(expected, ret.float_value, 0.01f);
	}

	void IsEnum(cstring name, int expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Enum, ret.type->GetGenericType());
		Assert::AreEqual(name, ret.type->GetName());
		Assert::AreEqual(expected, ret.int_value);
	}

	void IsString(cstring expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::String, ret.type->GetGenericType());
		Assert::AreEqual(expected, ret.str_value);
	}
};

#define CA_TEST_CLASS(Name) 					        	\
namespace tests												\
{															\
TEST_CLASS(Name) 											\
{ 															\
	BEGIN_TEST_CLASS_ATTRIBUTE() 							\
		TEST_CLASS_ATTRIBUTE(L"TestCategory", L#Name) 	    \
	END_TEST_CLASS_ATTRIBUTE() 								\
															\
	TEST_METHOD_INITIALIZE(OnTestSetup) 					\
	{ 														\
		CleanupErrors();									\
		module = engine->CreateModule();                    \
		current_module = module;                            \
		Retval::current = &retval;                          \
	}														\
	TEST_METHOD_CLEANUP(OnTestTeardown) 					\
	{ 														\
		CleanupAsserts(); 									\
		module->Release();                                  \
		current_module = nullptr;                           \
		SetDecompile(false);								\
		SetResetParser(true);								\
		current_call_context = nullptr;                     \
	}														\
															\
	IModule* module;										\
	Retval retval;                                          \

#define CA_TEST_CLASS_END() }; }

#define TEST_METHOD_IGNORE(x) \
	BEGIN_TEST_METHOD_ATTRIBUTE(x) \
		TEST_IGNORE() \
	END_TEST_METHOD_ATTRIBUTE() \
	TEST_METHOD(x)
