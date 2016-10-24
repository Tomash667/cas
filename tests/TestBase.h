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
			static wstring ToString(const ReturnValue::Type& type)
			{
				switch(type)
				{
				case ReturnValue::Void:
					return L"void";
				case ReturnValue::Bool:
					return L"bool";
				case ReturnValue::Int:
					return L"int";
				case ReturnValue::Float:
					return L"float";
				default:
					return L"invalid";
				}
			}
		}
	}
}

const bool CI_MODE = ((_CI_MODE - 1) == 0);

extern IModule* current_module;

void RunFileTest(IModule* module, cstring filename, cstring input, cstring output, bool optimize = true);
inline void RunFileTest(cstring filename, cstring input, cstring output, bool optimize = true)
{
	RunFileTest(current_module, filename, input, output, optimize);
}

void RunTest(IModule* module, cstring code);
inline void RunTest(cstring code)
{
	RunTest(current_module, code);
}

void RunFailureTest(IModule* module, cstring code, cstring error);
inline void RunFailureTest(cstring code, cstring error)
{
	RunFailureTest(current_module, code, error);
}

void CleanupAsserts();

struct Retval
{
	Retval(IModule* _module = nullptr)
	{
		module = _module;
	}
	
	void IsVoid()
	{
		ReturnValue ret = module->GetReturnValue();
		Assert::AreEqual(ReturnValue::Void, ret.type);
	}

	void IsBool(bool expected)
	{
		ReturnValue ret = module->GetReturnValue();
		Assert::AreEqual(ReturnValue::Bool, ret.type);
		Assert::AreEqual(expected, ret.bool_value);
	}

	void IsInt(int expected)
	{
		ReturnValue ret = module->GetReturnValue();
		Assert::AreEqual(ReturnValue::Int, ret.type);
		Assert::AreEqual(expected, ret.int_value);
	}

	void IsFloat(float expected)
	{
		ReturnValue ret = module->GetReturnValue();
		Assert::AreEqual(ReturnValue::Float, ret.type);
		Assert::AreEqual(expected, ret.float_value);
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
		module = CreateModule();							\
		current_module = module;                            \
		retval = Retval(module);                            \
	}														\
	TEST_METHOD_CLEANUP(OnTestTeardown) 					\
	{ 														\
		CleanupAsserts(); 									\
		DestroyModule(module);								\
		current_module = nullptr;                           \
	}														\
															\
	IModule* module;										\
	Retval retval;                                          \

#define CA_TEST_CLASS_END() }; }
