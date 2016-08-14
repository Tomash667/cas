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
	void pow(int& a)
	{
		a = a*a;
	}

	TEST_CLASS(Code)
	{
	public:
		TEST_METHOD(ReturnValueToCode)
		{
			// void
			RunTest("return;");
			ReturnValue ret = def_module->GetReturnValue();
			Assert::AreEqual(ret.type, ReturnValue::Void);

			// bool
			RunTest("return true;");
			ret = def_module->GetReturnValue();
			Assert::AreEqual(ret.type, ReturnValue::Bool);
			Assert::AreEqual(ret.bool_value, true);

			// int
			RunTest("return 3;");
			ret = def_module->GetReturnValue();
			Assert::AreEqual(ret.type, ReturnValue::Int);
			Assert::AreEqual(ret.int_value, 3);

			// float
			RunTest("return 3.14;");
			ret = def_module->GetReturnValue();
			Assert::AreEqual(ret.type, ReturnValue::Float);
			Assert::AreEqual(ret.float_value, 3.14f);
		}

		TEST_METHOD(MultipleReturnValueToCode)
		{
			// will upcast to common type - float
			RunTest("return 7; return 14.11; return false;");
			ReturnValue ret = def_module->GetReturnValue();
			Assert::AreEqual(ret.type, ReturnValue::Float);
			Assert::AreEqual(ret.float_value, 7.f);
		}

		TEST_METHOD(CodeFunctionTakesRef)
		{
			IModule* module = CreateModule();
			module->AddFunction("void pow(int& a)", pow);
			RunTest(module, "int a = 3; pow(a); return a;");
			module->ParseAndRun("int a = 3; pow(a); return a;");
			ReturnValue ret = module->GetReturnValue();
			Assert::AreEqual(ret.type, ReturnValue::Int);
			Assert::AreEqual(ret.int_value, 9);
			DestroyModule(module);
		}
	};
}
