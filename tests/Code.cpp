#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

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

namespace tests
{
	int global_a;
	int global_b;

	void pow(int& a)
	{
		a = a*a;
	}

	int& getref(bool is_a)
	{
		if(is_a)
			return global_a;
		else
			return global_b;
	}

	TEST_CLASS(Code)
	{
	public:
		TEST_METHOD(ReturnValueToCode)
		{
			// void
			RunTest("return;");
			ReturnValue ret = def_module->GetReturnValue();
			Assert::AreEqual(ReturnValue::Void, ret.type);

			// bool
			RunTest("return true;");
			ret = def_module->GetReturnValue();
			Assert::AreEqual(ReturnValue::Bool, ret.type);
			Assert::AreEqual(true, ret.bool_value);

			// int
			RunTest("return 3;");
			ret = def_module->GetReturnValue();
			Assert::AreEqual(ReturnValue::Int, ret.type);
			Assert::AreEqual(3, ret.int_value);

			// float
			RunTest("return 3.14;");
			ret = def_module->GetReturnValue();
			Assert::AreEqual(ReturnValue::Float, ret.type);
			Assert::AreEqual(3.14f, ret.float_value);
		}

		TEST_METHOD(MultipleReturnValueToCode)
		{
			// will upcast to common type - float
			RunTest("return 7; return 14.11; return false;");
			ReturnValue ret = def_module->GetReturnValue();
			Assert::AreEqual(ReturnValue::Float, ret.type);
			Assert::AreEqual(7.f, ret.float_value);
		}

		TEST_METHOD(CodeFunctionTakesRef)
		{
			IModule* module = CreateModule();
			module->AddFunction("void pow(int& a)", pow);
			RunTest(module, "int a = 3; pow(a); return a;");
			module->ParseAndRun("int a = 3; pow(a); return a;");
			ReturnValue ret = module->GetReturnValue();
			Assert::AreEqual(ReturnValue::Int, ret.type);
			Assert::AreEqual(9, ret.int_value);
			DestroyModule(module);
		}

		TEST_METHOD(CodeFunctionReturnsRef)
		{
			IModule* module = CreateModule();
			module->AddFunction("int& getref(bool is_a)", getref);
			global_a = 1;
			global_b = 2;
			RunTest(module, "getref(true) = 7; getref(false) *= 3;");
			Assert::AreEqual(7, global_a);
			Assert::AreEqual(6, global_b);
		}
	};
}
