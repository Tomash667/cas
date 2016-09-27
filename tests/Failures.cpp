#include "Pch.h"
#include "TestBase.h"

class Failures : public TestBase
{
public:
	void RunFailureTest(cstring code, cstring error)
	{
		env.ResetIO();
		
		Result result = ParseAndRunWithTimeout(module, code, true);
		string output = env.s_output.str();

		switch(result)
		{
		case Result::OK:
			ADD_FAILURE() << "Failure without error.";
			break;
		case Result::TIMEOUT:
			ADD_FAILURE() << Format("Script execution/parsing timeout. Parse output: [%s]", env.event_output.c_str());
			break;
		case Result::ASSERT:
			ADD_FAILURE() << Format("Code assert: [%s]", env.event_output.c_str());
			break;
		case Result::FAILED:
			{
				cstring out = env.event_output.c_str();
				cstring r = strstr(out, error);
				if(!r)
					ADD_FAILURE() << Format("Invalid error message. Expected:<%s> Actual:<%s>", error, out);
			}
			break;
		}
	}
};

TEST_F(Failures, FunctionNoReturnValue)
{
	RunFailureTest("int f(){}", "Function 'int f()' not always return value.");
}

TEST_F(Failures, ReturnReferenceToLocal)
{
	RunFailureTest("int& f() { int a; return a; }", "Returning reference to temporary variable 'int a'.");
	RunFailureTest("int& f(int a) { return a; }", "Returning reference to temporary variable 'int a'.");
}

TEST_F(Failures, ItemNameAlreadyUsed)
{
	RunFailureTest("int f, f;", "Name 'f' already used as global variable.");
	RunFailureTest("void f(int a) { int a; }", "Name 'a' already used as argument.");
	RunFailureTest("void f() { int a, a; }", "Name 'a' already used as local variable.");
}

TEST_F(Failures, MemberNameAlreadyUsed)
{
	RunFailureTest("class A { int a, a; }", "Member with name 'A.a' already exists.");
}

TEST_F(Failures, NoMatchingCallToFunction)
{
	RunFailureTest("void f(){} f(1);", "No matching call to function 'f' with arguments (int), could be 'void f()'.");
}

TEST_F(Failures, NoMatchingCallToMethod)
{
	RunFailureTest("class A { void f(){} } A a; a.f(1);", "No matching call to method 'A.f' with arguments (int), could be 'void A.f()'.");
}

TEST_F(Failures, AmbiguousCallToFunction)
{
	RunFailureTest("void f(bool b){} void f(float g){} f(1);",
		"Ambiguous call to overloaded function 'f' with arguments (int), could be:\n\tvoid f(bool)\n\tvoid f(float)");
}

TEST_F(Failures, AmbiguousCallToMethod)
{
	RunFailureTest("class A { void f(bool b){} void f(float g){} } A a; a.f(1);",
		"Ambiguous call to overloaded method 'A.f' with arguments (int), could be:\n\tvoid A.f(bool)\n\tvoid A.f(float)");
}

TEST_F(Failures, MissingConstructor)
{
	RunFailureTest("class A {}  A a = A(1,2,3);", "Type 'A' don't have constructor.");
}

TEST_F(Failures, MissingDefaultValue)
{
	RunFailureTest("void f(int a=0,float b){}", "Missing default value for argument 'b'.");
}

TEST_F(Failures, InvalidAssignType)
{
	RunFailureTest("class A{} A a; int b = a;", "Can't assign type 'A' to variable 'int b'.");
}

TEST_F(Failures, InvalidConditionType)
{
	RunFailureTest("class A{} A a; if(a) {}", "Condition expression with 'A' type.");
	RunFailureTest("class A{} A a; for(;a;) {}", "Condition expression with 'A' type.");
}

TEST_F(Failures, VoidVariable)
{
	RunFailureTest("void a;", "Can't declare void variable.");
	RunFailureTest("class A{void a;}", "Class member can't be void type.");
}

TEST_F(Failures, ReferenceToReferenceType)
{
	RunFailureTest("class A{} void f(A& a){}", "Can't create reference to reference type 'A'.");
}

TEST_F(Failures, InvalidDefaultValue)
{
	RunFailureTest("void f(int a=\"dodo\"){}", "Invalid default value of type 'string', required 'int'.");
}

TEST_F(Failures, UnsupportedClassMembers)
{
	RunFailureTest("class A{} class B{A a;}", "Class 'A' member not supported yet.");
	RunFailureTest("class A{string s;}", "Class 'string' member not supported yet.");
}

TEST_F(Failures, InvalidBreak)
{
	RunFailureTest("break;", "Not in breakable block.");
}

TEST_F(Failures, InvalidReturnType)
{
	RunFailureTest("int f() { return \"dodo\";", "Invalid return type 'string', function 'int f()' require 'int' type.");
}

TEST_F(Failures, InvalidClassDeclaration)
{
	RunFailureTest("void f() { class A{} }", "Class can't be declared inside block.");
}

TEST_F(Failures, InvalidFunctionDeclaration)
{
	RunFailureTest("void f() { void g() {} }", "Function can't be declared inside block.");
	RunFailureTest("{ void g() {} }", "Function can't be declared inside block.");
}

TEST_F(Failures, FunctionRedeclaration)
{
	RunFailureTest("void f(){} void f(){}", "Function 'void f()' already exists.");
	RunFailureTest("class A{ void f(){} void f(){} }", "Function 'void A.f()' already exists.");
}

TEST_F(Failures, ReferenceVariableUnavailable)
{
	RunFailureTest("int& a;", "Reference variable unavailable yet.");
}

TEST_F(Failures, InvalidOperationTypes)
{
	RunFailureTest("void f(){} 3+f();", "Invalid types 'int' and 'void' for operation 'add'.");
	RunFailureTest("void f(){} -f();", "Invalid type 'void' for operation 'unary minus'.");
	RunFailureTest("bool b; ++b;", "Invalid type 'bool' for operation 'pre increment'.");
	RunFailureTest("int a; a+=\"dodo\";", "Can't cast return value from 'string' to 'int' for operation 'assign add'.");
	RunFailureTest("void f(int& a){a+=\"dodo\";}", "Can't cast return value from 'string' to 'int' for operation 'assign add'.");
}

TEST_F(Failures, RequiredVariable)
{
	RunFailureTest("++3;", "Operation 'pre increment' require variable.");
	RunFailureTest("3 = 1;", "Can't assign, left value must be variable.");
}

TEST_F(Failures, InvalidMemberAccess)
{
	RunFailureTest("void f(){} f().dodo();", "Invalid member access for type 'void'.");
	RunFailureTest("3.ok();", "Missing method 'ok' for type 'int'.");
	RunFailureTest("class A{} A a; a.ok();", "Missing method 'ok' for type 'A'.");
	RunFailureTest("class A{} A a; a.b=1;", "Missing member 'b' for type 'A'.");
}

TEST_F(Failures, InvalidAssignTypes)
{
	RunFailureTest("int a; a=\"dodo\";", "Can't assign 'string' to type 'int'.");
	RunFailureTest("void f(int& a) {a=\"dodo\";}", "Can't assign 'string' to type 'int&'.");
}

TEST_F(Failures, MixedGlobalReturnType)
{
	RunFailureTest("return; return 1;", "Mismatched return type 'void' and 'int'.");
}

TEST_F(Failures, InvalidGlobalReturnType)
{
	RunFailureTest("return \"dada\";", "Invalid type 'string' for global return.");
	RunFailureTest("class A{} A a; return a;", "Invalid type 'A' for global return.");
	RunFailureTest("int& f(int& a){return a;} int a; return f(a);", "Invalid type 'int&' for global return.");
}
