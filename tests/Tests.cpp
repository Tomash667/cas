#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

#undef RegisterClass

namespace tests
{
	TEST_CLASS(Tests)
	{
		TEST_CATEGORY(Tests);
		TEST_METHOD_CLEANUP(Cleanup)
		{
			CleanupAsserts();
		}

		TEST_METHOD(RegisterClass)
		{
		}

		TEST_METHOD(RegisterStruct)
		{

		}
	};
}
