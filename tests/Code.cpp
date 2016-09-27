#include "Pch.h"
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

	class Code : public TestBase
	{
	public:
	};

	TEST_F(Code, ReturnValueToCode)
	{
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

	TEST_F(Code, MultipleReturnValueToCode)
	{
		// will upcast to common type - float
		RunTest("return 7; return 14.11; return false;");
		ret.IsFloat(7.f);
	}

	TEST_F(Code, CodeFunctionTakesRef)
	{
		module->AddFunction("void pow(int& a)", pow);
		RunTest("int a = 3; pow(a); return a;");
		ret.IsInt(9);
	}

	TEST_F(Code, CodeFunctionReturnsRef)
	{
		module->AddFunction("int& getref(bool is_a)", getref);
		global_a = 1;
		global_b = 2;
		RunTest("getref(true) = 7; getref(false) *= 3;");
		ASSERT_EQ(global_a, 7);
		ASSERT_EQ(global_b, 6);
	}

	TEST_F(Code, IsCompareCodeRefs)
	{
		module->AddFunction("int& getref(bool is_a)", getref);

		RunTest("return getref(true) is getref(true);");
		ret.IsBool(true);

		RunTest("return getref(true) is getref(false);");
		ret.IsBool(false);
	}
}
