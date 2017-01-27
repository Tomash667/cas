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

TEST_METHOD(StaticMethods)
{
	RunTest(R"code(
		int side_effect = 0;

		class A
		{
			int f(int a, int b)
			{
				return sum(a,b);
			}

			static int sum(int a, int b)
			{
				return a+b;
			}

			static void other()
			{
				int ga = sum(1,3);
				Assert_AreEqual(4, ga);
			}
	
			A side_effect_func()
			{
				side_effect = 1;
				A a;
				return a;
			}
		}

		int r = A.sum(1,2);
		Assert_AreEqual(3, r);

		A a;
		r = a.sum(2,4);
		Assert_AreEqual(6, r);

		r = a.side_effect_func().sum(3,5);
		Assert_AreEqual(8, r);
		Assert_AreEqual(1, side_effect);

		r = a.f(3,7);
		Assert_AreEqual(10, r);
		
		A.other();
	)code");
}

TEST_METHOD(EnumGlobalReturnValue)
{
	SetDecompile(true);
	SetResetParser(false);
	RunTest("enum E{A=2,B=4,C=6} return E.B;");
	retval.IsEnum("E", 4);

	module->ResetParser();
	IEnum* enu = module->AddEnum("F");
	enu->AddValues({ {"A", 3}, {"B", 6}, {"C", 9} });
	RunTest("return F.B;");
	retval.IsEnum("F", 6);
}

TEST_METHOD(IndexOperatorOnClass)
{
	RunTest(R"code(
		class A
		{
			int x, y, z;
			A(int _x, int _y, int _z)
			{
				x = _x;
				y = _y;
				z = _z;
			}
			int& operator [] (int n)
			{
				switch(n)
				{
				default:
				case 0:
					return x;
				case 1:
					return y;
				case 2:
					return z;
				}
			}
		}

		A a = A(1,2,3);
		a[0] += 4;
		a[1] = 7;
		a[2] *= a[0];

		Assert_AreEqual(5, a[0]);
		Assert_AreEqual(7, a[1]);
		Assert_AreEqual(15, a[2]);
	)code");
}

TEST_METHOD(ComplexSubscriptOperator)
{
	RunTest(R"code(
		string a, b, c;

		class A
		{
			A()
			{
				a = "test";
				b = "dodo";
				c = "dadar";
			}
	
			char& operator [] (int x, int y)
			{
				switch(x)
				{
				case 0:
				default:
					return a[y];
				case 1:
					return b[y];
				case 2:
					return c[y];
				}
			}
		}

		A aa;
		Assert_AreEqual('t', aa[0,0]);
		Assert_AreEqual('o', aa[1,1]);
		Assert_AreEqual('d', aa[2,2]);

		int x = getint(), y = getint();
		println(aa[x,y]);

		x = getint();
		y = getint();
		println(aa[x,y]);
	)code", "0 2 2 4", "s\nr\n");
}

TEST_METHOD(Dtor)
{

}

CA_TEST_CLASS_END();
