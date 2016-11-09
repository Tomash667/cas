#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

CA_TEST_CLASS(Tests);

TEST_METHOD(OverloadOperatorInScript)
{
	RunTest(R"code(
	class A {
		int x;
		int operator += (int a) { x += a; return x; }
	}

	A a; a.x = 7;
	int b = a += 3;
	Assert_AreEqual(10, b);
	Assert_AreEqual(10, a.x);

	)code");
}

CA_TEST_CLASS_END();
