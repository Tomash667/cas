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
	)code")
		.ShouldReturn([](Retval& r) { r.IsInt(1); });
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
		INT2& s = r += INT2(5,6);
		Assert_IsTrue(b is r && r is s);
		Assert_IsTrue(a == b && b == r);
	)code");
}

static int& return_ref_to_passed_ref(int& a)
{
	return a;
}
TEST_METHOD(ReturnReferenceToPassedSimpleRef)
{
	module->AddFunction("int& f(int& a)", return_ref_to_passed_ref);
	RunTest(R"code(
		int a = 3;
		int& b = a;
		int& r = f(b);
		r += 2;
		int& s = f(r);
		s += 3;
		Assert_IsTrue(b is r && r is s);
		Assert_IsTrue(a == b && b == r);
		Assert_AreEqual(8, a);
	)code");
}

static string& return_ref_to_passed_str(string& s)
{
	s += "34";
	return s;
}
TEST_METHOD(ReturnReferenceToPassedString)
{
	module->AddFunction("string& f(string& s)", return_ref_to_passed_str);
	RunTest(R"code(
		string a = "12";
		string& b = a;
		string& r = f(b);
		r += "56";
		string& s = f(r);
		s += "78";
		Assert_IsTrue(b is r && r is s);
		Assert_IsTrue(a == b && b == r);
		Assert_AreEqual("1234563478", a);
	)code");
}

class A
{
public:
	int x;
};
static A& ret_passed_class(A& a)
{
	a.x += 1;
	return a;
}
TEST_METHOD(VerifyClassReturnIsSameAsPassed)
{
	auto type = module->AddType<A>("A");
	type->AddMember("int x", offsetof(A, x));
	module->AddFunction("A& f(A& a)", ret_passed_class);
	RunTest(R"code(
		A a;
		a.x = 1;
		A b = f(a);
		Assert_IsTrue(a is b);
		Assert_AreEqual(2, b.x);
	)code");
}

TEST_METHOD(CallCtorInsideMethod)
{
	RunTest(R"code(
		class A
		{
			int x;
	
			A clone_me()
			{
				A clone;
				clone.x = x;
				return clone;
			}
		}
		A a;
		a.x = 1;
		A b = a.clone_me();
		Assert_AreEqual(1, b.x);
	)code");
}

TEST_METHOD(ReturnInsideIf)
{
	RunTest(R"code(
		int a = getint();
		if(a == 7)
			return 3.33;
	)code")
		.WithInput("1");
}

TEST_METHOD(SubscriptOperatorOnReference)
{
	RunTest(R"code(
		void f(string& s)
		{
			s[0] = 'a';	
		}

		string s = "test";
		f(s);
		Assert_AreEqual("aest", s);
	)code");
}

TEST_METHOD(SubscriptOperatorOnClassReference)
{
	RunTest(R"code(
		class A
		{
			int operator [] (int n) { return n; }
		}

		A a;
		Assert_AreEqual(100, a[100]);

		void f(A& aa)
		{
			Assert_AreEqual(200, a[200]);
		}(a);
	)code");
}

TEST_METHOD(CallOperatorOnClassReference)
{
	RunTest(R"code(
		class X
		{
			void operator () (){}
		}

		void f(X& x)
		{
			x();
		}

		X x;
		f(x);
	)code");
}

TEST_METHOD(MultipleFilesAndFunctions)
{
	module->Parse(R"code(
		void f1() { print("1"); }
		void f2() { print("2"); }
		f1();
		f2();
	)code");
	module->Parse(R"code(
		void f3() { print("3"); }
		void f4() { print("4"); }
		f1();
		f3();
		f4();
	)code");
	module->Parse(R"code(
		void f5() { print("5"); }
		void f6() { print("6"); }
		f2();
		f5();
		f3();
		f6();
	)code");

	RunTest().ShouldOutput("121342536");
}

TEST_METHOD(EmptyConstructorWithMembers)
{
	RunTest("class A{int x;A(){}}");
}

TEST_METHOD(CompoundAssignmentOnReturnClass)
{
	RunTest(R"code(
		class A
		{
			int i;
			A(int _i) { i = _i; }
			void operator += (int x) { print(i+"+="+x); }
		}
		A(1) += 4;
	)code")
		.ShouldOutput("1+=4");
}

CA_TEST_CLASS_END();
