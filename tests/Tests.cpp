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

TEST_METHOD(PassStringByRef)
{
	RunTest(R"code(
		void f(string& s)
		{
			s += "da";
		}

		void f2(string& s)
		{
			s = "elo";
		}

		string str = "do";
		f(str);
		Assert_AreEqual("doda", str);
		f2(str);
		Assert_AreEqual("elo", str);
	)code");
}

TEST_METHOD(ReturnStringByReference)
{
	RunTest(R"code(
		string global_s;
		string& f() { return global_s; }
		global_s = "123";
		f() += "456";
		Assert_AreEqual("123456", global_s);
	)code");
}

TEST_METHOD(StringReferenceVar)
{
	RunTest(R"code(
		string global_s;
		global_s = "123";
		void f()
		{
			string& s = global_s;
			s += "456";
		}
		f();
		Assert_AreEqual("123456", global_s);
	)code");
}

CA_TEST_CLASS_END();
