#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace tests
{
	TEST_CLASS(Fixes)
	{
	public:
		TEST_METHOD(CallingEmptyFunctionInfiniteLoop)
		{
			/*
			this caused something like this
			f:
				; missing ret
			main:
				call f
			*/
			RunTest("void f() {} f();");
		}

		TEST_METHOD(ReturnInVoidFunction)
		{
			RunTest("void f() {return;}");
		}
	};
}
