#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

CA_TEST_CLASS(Fixes);

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

TEST_METHOD(PostIncMember)
{
	RunTest("class X{int a;} X x; x.a++;");
}

TEST_METHOD(ReturnReferenceCompoundAssignment)
{
	RunTest(R"code(
		class X{int a;}
		X g;
		g.a = 7;
		X f() { return g; }
		f().a += 3;
		Assert_AreEqual(11, f().a+1);
		int a; a+=f().a+=3;
		Assert_AreEqual(13, a);
	)code");
}

TEST_METHOD(ReturnReferenceIncrement)
{
	RunTest(R"code(
		class X { int a; }
		X f(X x) { x.a = 4; return x; }

		X a;
		int t = ++f(a).a;
		Assert_AreEqual(5, a.a);
		Assert_AreEqual(5, t);

		a.a = 0;
		t = f(a).a++;
		Assert_AreEqual(5, a.a);
		Assert_AreEqual(4, t);
	)code");
}

TEST_METHOD(StringEqualOperator)
{
	RunTest(R"code(
		string a = "dada";
		string b = "dada";
		if(a == b)
			return 1;
		return 0;
	)code");
	retval.IsInt(1);
}

struct INT2
{
	int x, y;
	inline INT2(int x, int y) : x(x), y(y) {}
	inline INT2& operator += (const INT2& i)
	{
		x += i.x;
		y += i.y;
		return *this;
	}
};
TEST_METHOD(ReturnReferenceToPassedStruct)
{
	auto type = module->AddType<INT2>("INT2", cas::ValueType);
	type->AddMember("int x", offsetof(INT2, x));
	type->AddMember("int y", offsetof(INT2, y));
	type->AddCtor<int, int>("INT2(int x, int y)");
	type->AddMethod("INT2& operator += (INT2& i)", &INT2::operator+=);
	RunTest(R"code(
		INT2 a = INT2(1,2);
		INT2& b = a;
		INT2& r = b += INT2(3,4);
		Assert_IsTrue(b is r);
		Assert_IsTrue(a == b && b == r);
	)code");
}

CA_TEST_CLASS_END();
