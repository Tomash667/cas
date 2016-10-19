#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace tests
{
	TEST_CLASS(Failures)
	{
		TEST_CATEGORY(Failures);
		TEST_METHOD_CLEANUP(Cleanup)
		{
			CleanupAsserts();
		}

		TEST_METHOD(FunctionNoReturnValue)
		{
			RunFailureTest("int f(){}", "Function 'int f()' not always return value.");
		}

		TEST_METHOD(ReturnReferenceToLocal)
		{
			RunFailureTest("int& f() { int a; return a; }", "Returning reference to temporary variable 'int a'.");
			RunFailureTest("int& f(int a) { return a; }", "Returning reference to temporary variable 'int a'.");
		}

		TEST_METHOD(ItemNameAlreadyUsed)
		{
			RunFailureTest("int f, f;", "Name 'f' already used as global variable.");
			RunFailureTest("void f(int a) { int a; }", "Name 'a' already used as argument.");
			RunFailureTest("void f() { int a, a; }", "Name 'a' already used as local variable.");
		}

		TEST_METHOD(MemberNameAlreadyUsed)
		{
			RunFailureTest("class A { int a, a; }", "Member with name 'A.a' already exists.");
		}

		TEST_METHOD(NoMatchingCallToFunction)
		{
			RunFailureTest("void f(){} f(1);", "No matching call to function 'f' with arguments (int), could be 'void f()'.");
		}

		TEST_METHOD(NoMatchingCallToMethod)
		{
			RunFailureTest("class A { void f(){} } A a; a.f(1);", "No matching call to method 'A.f' with arguments (int), could be 'void A.f()'.");
		}

		TEST_METHOD(AmbiguousCallToFunction)
		{
			RunFailureTest("void f(bool b){} void f(float g){} f(1);",
				"Ambiguous call to overloaded function 'f' with arguments (int), could be:\n\tvoid f(bool)\n\tvoid f(float)");
		}

		TEST_METHOD(AmbiguousCallToMethod)
		{
			RunFailureTest("class A { void f(bool b){} void f(float g){} } A a; a.f(1);",
				"Ambiguous call to overloaded method 'A.f' with arguments (int), could be:\n\tvoid A.f(bool)\n\tvoid A.f(float)");
		}

		TEST_METHOD(MissingConstructor)
		{
			RunFailureTest("class A {}  A a = A(1,2,3);", "Type 'A' don't have constructor.");
		}

		TEST_METHOD(MissingDefaultValue)
		{
			RunFailureTest("void f(int a=0,float b){}", "Missing default value for argument 'b'.");
		}

		TEST_METHOD(InvalidAssignType)
		{
			RunFailureTest("class A{} A a; int b = a;", "Can't assign type 'A' to variable 'int b'.");
		}

		TEST_METHOD(InvalidConditionType)
		{
			RunFailureTest("class A{} A a; if(a) {}", "Condition expression with 'A' type.");
			RunFailureTest("class A{} A a; for(;a;) {}", "Condition expression with 'A' type.");
		}

		TEST_METHOD(VoidVariable)
		{
			RunFailureTest("void a;", "Can't declare void variable.");
			RunFailureTest("class A{void a;}", "Class/struct member can't be void type.");
		}

		TEST_METHOD(ReferenceToReferenceType)
		{
			RunFailureTest("class A{} void f(A& a){}", "Can't create reference to reference type 'A'.");
		}

		TEST_METHOD(InvalidDefaultValue)
		{
			RunFailureTest("void f(int a=\"dodo\"){}", "Invalid default value of type 'string', required 'int'.");
		}

		TEST_METHOD(UnsupportedClassMembers)
		{
			RunFailureTest("class A{} class B{A a;}", "Class/struct member of type 'class' not supported yet.");
			RunFailureTest("class A{string s;}", "Class/struct member of type 'string' not supported yet.");
		}

		TEST_METHOD(InvalidBreak)
		{
			RunFailureTest("break;", "Not in breakable block.");
		}
		
		TEST_METHOD(InvalidReturnType)
		{
			RunFailureTest("int f() { return \"dodo\";", "Invalid return type 'string', function 'int f()' require 'int' type.");
		}

		TEST_METHOD(InvalidClassDeclaration)
		{
			RunFailureTest("void f() { class A{} }", "Class can't be declared inside block.");
		}

		TEST_METHOD(InvalidFunctionDeclaration)
		{
			RunFailureTest("void f() { void g() {} }", "Function can't be declared inside block.");
			RunFailureTest("{ void g() {} }", "Function can't be declared inside block.");
		}

		TEST_METHOD(FunctionRedeclaration)
		{
			RunFailureTest("void f(){} void f(){}", "Function 'void f()' already exists.");
			RunFailureTest("class A{ void f(){} void f(){} }", "Function 'void A.f()' already exists.");
		}

		TEST_METHOD(ReferenceVariableUnavailable)
		{
			RunFailureTest("int& a;", "Reference variable unavailable yet.");
		}

		TEST_METHOD(InvalidOperationTypes)
		{
			RunFailureTest("void f(){} 3+f();", "Invalid types 'int' and 'void' for operation 'add'.");
			RunFailureTest("void f(){} -f();", "Invalid type 'void' for operation 'unary minus'.");
			RunFailureTest("bool b; ++b;", "Invalid type 'bool' for operation 'pre increment'.");
			RunFailureTest("int a; a+=\"dodo\";", "Can't cast return value from 'string' to 'int' for operation 'assign add'.");
			RunFailureTest("void f(int& a){a+=\"dodo\";}", "Can't cast return value from 'string' to 'int' for operation 'assign add'.");
		}

		TEST_METHOD(RequiredVariable)
		{
			RunFailureTest("++3;", "Operation 'pre increment' require variable.");
			RunFailureTest("3 = 1;", "Can't assign, left value must be variable.");
		}

		TEST_METHOD(InvalidMemberAccess)
		{
			RunFailureTest("void f(){} f().dodo();", "Invalid member access for type 'void'.");
			RunFailureTest("3.ok();", "Missing method 'ok' for type 'int'.");
			RunFailureTest("class A{} A a; a.ok();", "Missing method 'ok' for type 'A'.");
			RunFailureTest("class A{} A a; a.b=1;", "Missing member 'b' for type 'A'.");
		}

		TEST_METHOD(InvalidAssignTypes)
		{
			RunFailureTest("int a; a=\"dodo\";", "Can't assign 'string' to type 'int'.");
			RunFailureTest("void f(int& a) {a=\"dodo\";}", "Can't assign 'string' to type 'int&'.");
		}

		TEST_METHOD(MixedGlobalReturnType)
		{
			RunFailureTest("return; return 1;", "Mismatched return type 'void' and 'int'.");
		}

		TEST_METHOD(InvalidGlobalReturnType)
		{
			RunFailureTest("return \"dada\";", "Invalid type 'string' for global return.");
			RunFailureTest("class A{} A a; return a;", "Invalid type 'A' for global return.");
			RunFailureTest("int& f(int& a){return a;} int a; return f(a);", "Invalid type 'int&' for global return.");
		}
	};
}
