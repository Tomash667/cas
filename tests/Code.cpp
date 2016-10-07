#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

namespace tests
{
	TEST_CLASS(Code)
	{
	public:
		//=========================================================================================
		TEST_METHOD(ReturnValueToCode)
		{
			Retval ret;

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

		//=========================================================================================
		TEST_METHOD(MultipleReturnValueToCode)
		{
			// will upcast to common type - float
			RunTest("return 7; return 14.11; return false;");
			Retval ret;
			ret.IsFloat(7.f);
		}

		//=========================================================================================
		static void pow(int& a)
		{
			a = a*a;
		}

		TEST_METHOD(CodeFunctionTakesRef)
		{
			ModuleRef module;
			module->AddFunction("void pow(int& a)", pow);
			module.RunTest("int a = 3; pow(a); return a;");
			module.ret().IsInt(9);
		}

		//=========================================================================================
		static int global_a;
		static int global_b;
		
		static int& getref(bool is_a)
		{
			if(is_a)
				return global_a;
			else
				return global_b;
		}

		TEST_METHOD(CodeFunctionReturnsRef)
		{
			ModuleRef module;
			module->AddFunction("int& getref(bool is_a)", getref);
			global_a = 1;
			global_b = 2;
			module.RunTest("getref(true) = 7; getref(false) *= 3;");
			Assert::AreEqual(7, global_a);
			Assert::AreEqual(6, global_b);
		}

		//=========================================================================================
		TEST_METHOD(IsCompareCodeRefs)
		{
			ModuleRef module;
			module->AddFunction("int& getref(bool is_a)", getref);

			module.RunTest("return getref(true) is getref(true);");
			module.ret().IsBool(true);

			module.RunTest("return getref(true) is getref(false);");
			module.ret().IsBool(false);
		}

		//=========================================================================================
		static int f1() { return 1; }
		static int f1(int i) { return i; }

		TEST_METHOD(CodeRegisterOverloadedFunctions)
		{
			ModuleRef module;
			module->AddFunction("int f1()", AsFunction(f1, int, ()));
			module->AddFunction("int f1(int i)", AsFunction(f1, int, (int)));
			module.RunTest("return f1() + f1(3) == 4;");
			module.ret().IsBool(true);
		}

		//=========================================================================================
		class AObj
		{
		public:
			int x;

			inline int GetX() { return x; }
			inline void SetX(int _x) { x = _x; }
		};

		TEST_METHOD(CodeMemberFunction)
		{
			ModuleRef module;
			module->AddType<AObj>("AObj");
			module->AddMethod("AObj", "int GetX()", &AObj::GetX);
			module->AddMethod("AObj", "void SetX(int a)", &AObj::SetX);
			module.RunTest("AObj a; a.SetX(7); return a.GetX();");
			module.ret().IsInt(7);
		}

		//=========================================================================================
		class BObj
		{
		public:
			int x;

			inline int f() { return 1; }
			inline int f(int i) { return i * 2; }
		};

		TEST_METHOD(CodeMemberFunctionOverload)
		{
			ModuleRef module;
			module->AddType<CObj>("BObj");
			module->AddMethod("BObj", "int f()", AsMethod(BObj, f, int, ()));
			module->AddMethod("BObj", "int f(int a)", AsMethod(BObj, f, int, (int)));
			module.RunTest("BObj b; return b.f() + b.f(4);");
			module.ret().IsInt(9);
		}

		//=========================================================================================
		class CObj
		{
		public:
			int x;

			inline CObj() : x(1) {}
			inline CObj(int x) : x(x) {}
			inline int GetX() { return x; }
		};

		struct AsCtorHelper
		{
			template<typename T, typename... Args>
			static T* Create(Args&&... args)
			{
				return new T(args...);
			}
		};

		template<typename T, typename... Args>
		inline FunctionInfo AsCtor()
		{
			return FunctionInfo(AsCtorHelper::Create<T, Args...>);
		}

		TEST_METHOD(CodeCtorMemberFunction)
		{
			ModuleRef module;
			module->AddType<CObj>("CObj");
			module->AddMethod("CObj", "CObj()", AsCtor<CObj>());
			module->AddMethod("CObj", "CObj(int a)", AsCtor<CObj, int>());
			module->AddMethod("CObj", "int GetX()", &CObj::GetX);
			module.RunTest("CObj c1; CObj c7 = CObj(7); return c1.GetX() + c7.GetX();");
			module.ret().IsInt(8);
		}


		// CodeCtorMemberFunctionOverload
	};

	int Code::global_a;
	int Code::global_b;
}
