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

TEST_METHOD(ClassMemberDefaultValue)
{
	RunTest(R"code(
		class X { int a = 4; }
		X x;
		Assert_AreEqual(4, x.a);
	)code");

	RunTest(R"code(
		class X { int a = 4, b; X() { b = a*2; a = 1; } }
		X x;
		Assert_AreEqual(1, x.a);
		Assert_AreEqual(8, x.b);
	)code");
}

CA_TEST_CLASS_END();
