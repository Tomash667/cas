#pragma once

#define TEST_CATEGORY(category) \
			BEGIN_TEST_CLASS_ATTRIBUTE() \
				TEST_CLASS_ATTRIBUTE(L"TestCategory", L#category) \
			END_TEST_CLASS_ATTRIBUTE()

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
}

void CleanupAsserts();

struct Retval
{
	Retval(IModule* _module = nullptr)
	{
		if(!_module)
			module = def_module;
		else
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
