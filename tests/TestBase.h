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

struct Retval;

typedef delegate<void(Retval&)> ReturnDelegate;

struct TestSettings
{
	IModule* module;
	cstring filename;
	cstring code;
	cstring input;
	cstring output;
	cstring error;
	ReturnDelegate ret_delegate;
	bool optimize;
	bool decompile;
	bool dont_reset;
};

void RunTest(TestSettings& settings);

struct RunTestProxy
{
public:
	RunTestProxy(cstring filename, cstring code)
	{
		settings.module = current_module;
		settings.filename = filename;
		settings.code = code;
		settings.input = nullptr;
		settings.output = nullptr;
		settings.error = nullptr;
		settings.optimize = true;
		settings.decompile = false;
		settings.dont_reset = false;
	}

	~RunTestProxy()
	{
		RunTest(settings);
	}

	RunTestProxy& ShouldFail(cstring error)
	{
		assert(error);
		settings.error = error;
		return *this;
	}

	RunTestProxy& WithInput(cstring input)
	{
		assert(input);
		settings.input = input;
		return *this;
	}

	RunTestProxy& ShouldOutput(cstring output)
	{
		assert(output);
		settings.output = output;
		return *this;
	}

	RunTestProxy& DontOptimize()
	{
		settings.optimize = false;
		return *this;
	}

	RunTestProxy& ShouldReturn(ReturnDelegate d)
	{
		settings.ret_delegate = d;
		return *this;
	}

	RunTestProxy& Decompile()
	{
		settings.decompile = true;
		return *this;
	}

	RunTestProxy& DontReset()
	{
		settings.dont_reset = true;
		return *this;
	}

private:
	TestSettings settings;
};

inline RunTestProxy RunFileTest(cstring filename)
{
	return RunTestProxy(filename, nullptr);
}

inline RunTestProxy RunTest(cstring code)
{
	return RunTestProxy(nullptr, code);
}

inline RunTestProxy RunTest()
{
	return RunTestProxy(nullptr, nullptr);
}

void CleanupErrors();
void CleanupOutput();
void AssertError(cstring error);
void AssertOutput(cstring output);
void WriteOutput(cstring msg);
void WriteDecompileOutput(IModule* module);

struct Retval
{
	ICallContext* call_context;

	Retval() {}
	Retval(ICallContext* call_context) : call_context(call_context) {}
	
	void IsVoid()
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Void, ret.type.generic_type);
	}

	void IsBool(bool expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Bool, ret.type.generic_type);
		Assert::AreEqual(expected, ret.bool_value);
	}

	void IsChar(char expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Char, ret.type.generic_type);
		Assert::AreEqual(expected, ret.char_value);
	}

	void IsInt(int expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Int, ret.type.generic_type);
		Assert::AreEqual(expected, ret.int_value);
	}

	void IsFloat(float expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Float, ret.type.generic_type);
		Assert::AreEqual(expected, ret.float_value, 0.01f);
	}

	void IsEnum(cstring name, int expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::Enum, ret.type.generic_type);
		Assert::AreEqual(name, ret.type.specific_type->GetName());
		Assert::AreEqual(expected, ret.int_value);
	}

	void IsString(cstring expected)
	{
		Value ret = call_context->GetReturnValue();
		Assert::AreEqual(GenericType::String, ret.type.generic_type);
		Assert::AreEqual(expected, ret.str_value);
	}

	IObject* GetObject()
	{
		Value ret = call_context->GetReturnValue();
		Assert::IsTrue(ret.type.generic_type == GenericType::Class || ret.type.generic_type == GenericType::Struct);
		return ret.obj;
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
	}														\
	TEST_METHOD_CLEANUP(OnTestTeardown) 					\
	{ 														\
		CleanupOutput();									\
		module->Release();                                  \
		current_module = nullptr;                           \
	}														\
															\
	IModule* module;										\

#define CA_TEST_CLASS_END() }; }

#define TEST_METHOD_IGNORE(x) \
	BEGIN_TEST_METHOD_ATTRIBUTE(x) \
		TEST_IGNORE() \
	END_TEST_METHOD_ATTRIBUTE() \
	TEST_METHOD(x)
