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
			static wstring ToString(const IType::GenericType& generic_type)
			{
				switch(generic_type)
				{
				case IType::GenericType::Void:
					return L"void";
				case IType::GenericType::Bool:
					return L"bool";
				case IType::GenericType::Char:
					return L"char";
				case IType::GenericType::Int:
					return L"int";
				case IType::GenericType::Float:
					return L"float";
				case IType::GenericType::String:
					return L"string";
				case IType::GenericType::Class:
					return L"class";
				case IType::GenericType::Struct:
					return L"struct";
				case IType::GenericType::Enum:
					return L"enum";
				case IType::GenericType::Invalid:
				default:
					return L"invalid";
				}
			}

			template<>
			static wstring ToString(const IModule::ExecutionResult& result)
			{
				switch(result)
				{
				case IModule::ValidationError:
					return L"ValidationError";
				case IModule::ParsingError:
					return L"ParsingError";
				case IModule::Exception:
					return L"Exception";
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

extern IModule* current_module;

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

inline void RunTest(cstring code, cstring input = "")
{
	TestSettings s;
	s.module = current_module;
	s.filename = nullptr;
	s.code = code;
	s.input = input;
	s.output = nullptr;
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

inline void RunParsedTest(cstring input = "")
{
	TestSettings s;
	s.module = current_module;
	s.filename = nullptr;
	s.code = nullptr;
	s.input = input;
	s.output = nullptr;
	s.error = nullptr;
	s.optimize = true;
	RunTest(s);
}

void CleanupErrors();
void CleanupAsserts();
void AssertError(cstring error);
void SetDecompile(bool decompile);
void SetResetParser(bool reset_parser);

struct Retval
{
	Retval(IModule* _module = nullptr)
	{
		module = _module;
	}
	
	void IsVoid()
	{
		ReturnValue ret = module->GetReturnValue();
		Assert::AreEqual(IType::GenericType::Void, ret.type->GetGenericType());
	}

	void IsBool(bool expected)
	{
		ReturnValue ret = module->GetReturnValue();
		Assert::AreEqual(IType::GenericType::Bool, ret.type->GetGenericType());
		Assert::AreEqual(expected, ret.bool_value);
	}

	void IsChar(char expected)
	{
		ReturnValue ret = module->GetReturnValue();
		Assert::AreEqual(IType::GenericType::Char, ret.type->GetGenericType());
		Assert::AreEqual(expected, ret.char_value);
	}

	void IsInt(int expected)
	{
		ReturnValue ret = module->GetReturnValue();
		Assert::AreEqual(IType::GenericType::Int, ret.type->GetGenericType());
		Assert::AreEqual(expected, ret.int_value);
	}

	void IsFloat(float expected)
	{
		ReturnValue ret = module->GetReturnValue();
		Assert::AreEqual(IType::GenericType::Float, ret.type->GetGenericType());
		Assert::AreEqual(expected, ret.float_value, 0.01f);
	}

	void IsEnum(cstring name, int expected)
	{
		ReturnValue ret = module->GetReturnValue();
		Assert::AreEqual(IType::GenericType::Enum, ret.type->GetGenericType());
		Assert::AreEqual(name, ret.type->GetName());
		Assert::AreEqual(expected, ret.int_value);
	}

private:
	IModule* module;
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
		module = CreateModule();							\
		current_module = module;                            \
		retval = Retval(module);                            \
	}														\
	TEST_METHOD_CLEANUP(OnTestTeardown) 					\
	{ 														\
		CleanupAsserts(); 									\
		DestroyModule(module);								\
		current_module = nullptr;                           \
		SetDecompile(false);								\
		SetResetParser(true);								\
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
