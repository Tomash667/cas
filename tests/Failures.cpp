#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace tests
{
	TEST_CLASS(Failures)
	{
	public:
		TEST_METHOD(FunctionNoReturnValue)
		{
			RunFailureTest("int f(){}", "Function 'int f()' not always return value.");
		}

		TEST_METHOD(ReturnReferenceToLocal)
		{
			RunFailureTest("int& f() { int a; return a; }", "Returning reference to local variable 'int a'.");
		}
	};
}
