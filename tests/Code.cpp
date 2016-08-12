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
			static std::wstring ToString(const cas::ReturnValue::Type& type)
			{
				switch(type)
				{
				case cas::ReturnValue::Void:
					return L"void";
				case cas::ReturnValue::Bool:
					return L"bool";
				case cas::ReturnValue::Int:
					return L"int";
				case cas::ReturnValue::Float:
					return L"float";
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
			cas::ReturnValue ret = cas::GetReturnValue();
			Assert::AreEqual(ret.type, cas::ReturnValue::Void);

			// bool
			RunTest("return true;");
			ret = cas::GetReturnValue();
			Assert::AreEqual(ret.type, cas::ReturnValue::Bool);
			Assert::AreEqual(ret.bool_value, true);

			// int
			RunTest("return 3;");
			ret = cas::GetReturnValue();
			Assert::AreEqual(ret.type, cas::ReturnValue::Int);
			Assert::AreEqual(ret.int_value, 3);

			// float
			RunTest("return 3.14;");
			ret = cas::GetReturnValue();
			Assert::AreEqual(ret.type, cas::ReturnValue::Float);
			Assert::AreEqual(ret.float_value, 3.14f);
		}

		TEST_METHOD(MultipleReturnValueToCode)
		{
			// will upcast to common type - float
			RunTest("return 7; return 14.11; return false;");
			cas::ReturnValue ret = cas::GetReturnValue();
			Assert::AreEqual(ret.type, cas::ReturnValue::Float);
			Assert::AreEqual(ret.float_value, 7.f);
		}

		TEST_METHOD(CodeFunctionTakesRef)
		{
			cas::Module* module = cas::GetModule();
			module->AddFunction("void pow(int& a)", pow);
			module->ParseAndRun("int a = 3; pow(a); return a;");
			cas::ReturnValue ret = module->GetReturnValue();
			Assert::AreEqual(ret.type, cas::ReturnValue::Int);
			Assert::AreEqual(ret.int_value, 9);
		}
	};
}
