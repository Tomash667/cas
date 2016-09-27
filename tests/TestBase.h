#pragma once

#include "TestEnvironment.h"

enum class Result
{
	OK,
	TIMEOUT,
	FAILED,
	ASSERT
};

struct Retval
{
	void Set(IModule* _module)
	{
		module = _module;
	}

	void IsVoid()
	{
		ReturnValue ret = module->GetReturnValue();
		ASSERT_EQ(ReturnValue::Void, ret.type);
	}

	void IsBool(bool expected)
	{
		ReturnValue ret = module->GetReturnValue();
		ASSERT_EQ(ReturnValue::Bool, ret.type);
		ASSERT_EQ(expected, ret.bool_value);
	}

	void IsInt(int expected)
	{
		ReturnValue ret = module->GetReturnValue();
		ASSERT_EQ(ReturnValue::Int, ret.type);
		ASSERT_EQ(expected, ret.int_value);
	}

	void IsFloat(float expected)
	{
		ReturnValue ret = module->GetReturnValue();
		ASSERT_EQ(ReturnValue::Float, ret.type);
		ASSERT_EQ(expected, ret.float_value);
	}

private:
	IModule* module;
};

class TestBase : public testing::Test
{
public:
	TestBase();
	void SetUp() override;
	void TearDown() override;

	Result ParseAndRunWithTimeout(IModule* module, cstring content, bool optimize, int timeout = -1);
	Result ParseAndRunChecked(IModule* module, cstring input, bool optimize);
	void RunTest(cstring code);
	
	IModule* module;
	TestEnvironment& env;
	Retval ret;
};

/*
using namespace cas;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			template<>
			static std::wstring ToString(const ReturnValue::Type& type)
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

extern IModule* def_module;

void RunFileTest(IModule* module, cstring filename, cstring input, cstring output, bool optimize = true);
inline void RunFileTest(cstring filename, cstring input, cstring output, bool optimize = true)
{
	RunFileTest(def_module, filename, input, output, optimize);
}

void RunTest(IModule* module, cstring code);
inline void RunTest(cstring code)
{
	RunTest(def_module, code);
}

void RunFailureTest(IModule* module, cstring code, cstring error);
inline void RunFailureTest(cstring code, cstring error)
{
	RunFailureTest(def_module, code, error);
}*/



/*
struct ModuleRef
{
	ModuleRef()
	{
		module = CreateModule();
	}

	~ModuleRef()
	{
		DestroyModule(module);
	}

	IModule* operator -> ()
	{
		return module;
	}

	Retval ret()
	{
		return Retval(module);
	}

	void RunTest(cstring code)
	{
		::RunTest(module, code);
	}
	
private:
	IModule* module;
};
*/
