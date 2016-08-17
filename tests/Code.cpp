#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

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
			Retval ret;

			// void
			RunTest("return;");
			ret.IsVoid();

			// bool
			RunTest("return true;");
			ret.IsBool(true);

			// int
			RunTest("return 3;");
			ret.IsInt(3);

			// float
			RunTest("return 3.14;");
			ret.IsFloat(3.14f);
		}

		TEST_METHOD(MultipleReturnValueToCode)
		{
			// will upcast to common type - float
			RunTest("return 7; return 14.11; return false;");
			Retval ret;
			ret.IsFloat(7.f);
		}

		TEST_METHOD(CodeFunctionTakesRef)
		{
			ModuleRef module;
			module->AddFunction("void pow(int& a)", pow);
			module.RunTest("int a = 3; pow(a); return a;");
			module.ret().IsInt(9);
		}

		TEST_METHOD(CodeFunctionReturnsRef)
		{
			ModuleRef module;
			module->AddFunction("int& getref(bool is_a)", getref);
			global_a = 1;
			global_b = 2;
			module.RunTest("getref(true) = 7; getref(false) *= 3;");
			Assert::AreEqual(7, global_a);
			Assert::AreEqual(6, global_b);
		}

		TEST_METHOD(IsCompareCodeRefs)
		{
			ModuleRef module;
			module->AddFunction("int& getref(bool is_a)", getref);

			module.RunTest("return getref(true) is getref(true);");
			module.ret().IsBool(true);

			module.RunTest("return getref(true) is getref(false);");
			module.ret().IsBool(false);
		}
	};
}
