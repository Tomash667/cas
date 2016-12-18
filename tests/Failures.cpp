#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class A
{
public:
	void f() {}
	int x, y;
};

class B
{
public:
	B(int a, int b) {}
};

static void f() {}

CA_TEST_CLASS(Failures);

TEST_METHOD(FunctionNoReturnValue)
{
	RunFailureTest("int f(){}", "Function 'int f()' not always return value.");
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
	RunFailureTest("class A {}  A a = A(1,2,3);", "No matching call to method 'A.A' with arguments (int,int,int), could be 'A.A()'.");
}

TEST_METHOD(MissingDefaultValue)
{
	RunFailureTest("void f(int a=0,float b){}", "Missing default value for argument 2 'float b'.");
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
	RunFailureTest("class A{void a;}", "Member of 'void' type not allowed.");
}

TEST_METHOD(InvalidDefaultValue)
{
	RunFailureTest("void f(int a=\"dodo\"){}", "Invalid default value of type 'string', required 'int'.");
}

TEST_METHOD(UnsupportedClassMembers)
{
	RunFailureTest("class A{} class B{A a;}", "Member of 'class' type not allowed yet.");
	RunFailureTest("class A{string s;}", "Member of 'string' type not allowed yet.");
}

TEST_METHOD(InvalidBreak)
{
	RunFailureTest("break;", "Not in breakable block.");
}
		
TEST_METHOD(InvalidReturnType)
{
	RunFailureTest("int f() { return \"dodo\"; }", "Invalid return type 'string', function 'int f()' require 'int' type.");
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
	RunFailureTest("class A{ void f(){} void f(){} }", "Method 'void A.f()' already exists.");
}

TEST_METHOD(ReferenceVariableUninitialized)
{
	RunFailureTest("int& a;", "Uninitialized reference variable.");
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
	RunFailureTest("3 = 1;", "Can't assign, left must be assignable.");
}

TEST_METHOD(InvalidMemberAccess)
{
	RunFailureTest("void f(){} f().dodo();", "Missing method 'dodo' for type 'void'.");
	RunFailureTest("3.ok();", "Missing method 'ok' for type 'int'.");
	RunFailureTest("class A{} A a; a.ok();", "Missing method 'ok' for type 'A'.");
	RunFailureTest("class A{} A a; a.b=1;", "Missing member 'b' for type 'A'.");
}

TEST_METHOD(InvalidAssignTypes)
{
	RunFailureTest("int a; a=\"dodo\";", "No matching call to method 'int.operator = ' with arguments (string), could be 'int int.operator = (int)'.");
	RunFailureTest("void f(int& a) {a=\"dodo\";}",
		"No matching call to method 'int.operator = ' with arguments (string), could be 'int int.operator = (int)'.");
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

TEST_METHOD(RegisterFunctionWithThiscall)
{
	bool r = module->AddFunction("void f()", &A::f);
	Assert::IsFalse(r);
	AssertError("Can't use thiscall in function 'void f()'.");
}

TEST_METHOD(RegisterFunctionParseError)
{
	bool r = module->AddFunction("void f;", f);
	Assert::IsFalse(r);
	AssertError("Expecting symbol '(', found symbol ';'.");
	AssertError("Failed to parse function declaration for AddFunction 'void f;'.");
}

TEST_METHOD(RegisterSameFunctionTwice)
{
	bool r = module->AddFunction("void f()", f);
	Assert::IsTrue(r);

	r = module->AddFunction("void f()", f);
	Assert::IsFalse(r);
	AssertError("Function 'void f()' already exists.");
}

TEST_METHOD(RegisterTypeIsKeyword)
{
	IType* type = module->AddType<A>("if");
	Assert::IsNull(type);
	AssertError("Can't declare type 'if', name is keyword.");
}

TEST_METHOD(RegisterSameTypeTwice)
{
	IType* type = module->AddType<A>("A");
	Assert::IsNotNull(type);

	type = module->AddType<A>("A");
	Assert::IsNull(type);
	AssertError("Type 'A' already declared.");
}

TEST_METHOD(RegisterMethodParseError)
{
	IType* type = module->AddType<A>("A");
	Assert::IsNotNull(type);

	bool r = type->AddMethod("void f;", &A::f);
	Assert::IsFalse(r);
	AssertError("Expecting symbol '(', found symbol ';'.");
	AssertError("Failed to parse function declaration for AddMethod 'void f;'.");
}

TEST_METHOD(RegisterSameMethodTwice)
{
	IType* type = module->AddType<A>("A");
	Assert::IsNotNull(type);

	bool r = type->AddMethod("void f()", &A::f);
	Assert::IsTrue(r);

	r = type->AddMethod("void f()", &A::f);
	Assert::IsFalse(r);
	AssertError("Method 'void f()' for type 'A' already exists.");
}

TEST_METHOD(RegisterMemberParseError)
{
	IType* type = module->AddType<A>("A");
	Assert::IsNotNull(type);

	bool r = type->AddMember("int;", offsetof(A, x));
	Assert::IsFalse(r);
	AssertError("Expecting item, found symbol ';'.");
	AssertError("Failed to parse member declaration for type 'A' AddMember 'int;'.");
}

TEST_METHOD(InvalidSubscriptIndexType)
{
	RunFailureTest("string a; a[a] = 'b';", "Subscript operator require type 'int', found 'string'.");
}

TEST_METHOD(InvalidSubscriptType)
{
	RunFailureTest("int a; a[0] = 1;", "Type 'int' don't have subscript operator.");
}

TEST_METHOD(OperatorFunctionOutsideClass)
{
	RunFailureTest("void operator += (int a) {}", "Operator function can be used only inside class.");
}

TEST_METHOD(CantOverloadOperator)
{
	RunFailureTest("class A{void operator . (){}}", "Can't overload operator '.'.");
}

TEST_METHOD(InvalidOperatorOverloadDefinition)
{
	RunFailureTest("class A{void operator + (int a, int b){}}", "Invalid overload operator definition 'void A.operator + (int,int)'.");
}

TEST_METHOD(AmbiguousCallToOverloadedOperator)
{
	RunFailureTest(R"code(
	class A{
		void operator += (int a) {}
		void operator += (string s) {}
	}
	A a;
	a += 1.13;
	)code",
		"Ambiguous call to overloaded method 'A.operator += ' with arguments (float), could be:\n\tvoid A.operator += (int)\n\tvoid A.operator += (string)");
}

TEST_METHOD(InvalidFunctorArgs)
{
	RunFailureTest("class A{ void operator () (int a) {}} A a; a(3, 14);",
		"No matching call to method 'A.operator () ' with arguments (int,int), could be 'void A.operator () (int)'.");
}

TEST_METHOD(InvalidFunctorType)
{
	RunFailureTest("int a = 4; a();", "Type 'int' don't have call operator.");
}

TEST_METHOD(InvalidTernaryCommonType)
{
	RunFailureTest("void f(){} 1?f():1;", "Invalid common type for ternary operator with types 'void' and 'int'.");
}

TEST_METHOD(InvalidTernaryCondition)
{
	RunFailureTest("void f(){} f()?0:1;", "Ternary condition expression with 'void' type.");
}

TEST_METHOD(InvalidSwitchType)
{
	RunFailureTest("class X{} X x; switch(x){}", "Invalid switch type 'X'.");
}

TEST_METHOD(CantCastCaseType)
{
	RunFailureTest("int a; switch(a){case \"a\":}", "Can't cast case value from 'string' to 'int'.");
}

TEST_METHOD(CaseAlreadyDefined)
{
	RunFailureTest("int a; switch(a){case 0:case 0:}", "Case with value '0' is already defined.");
}

TEST_METHOD(DefaultCaseAlreadyDefined)
{
	RunFailureTest("int a; switch(a){default:default:}", "Default case already defined.");
}

TEST_METHOD(BrokenSwitch)
{
	RunFailureTest("int a; switch(a){do}", "Expecting keyword 'case' from group 'keywords', keyword 'default' from group 'keywords', "
		"found keyword 'do' from group 'keywords'.");
}

TEST_METHOD(ImplicitFunction)
{
	RunFailureTest("implicit void f(){}", "Implicit can only be used for methods.");
}

TEST_METHOD(ImplicitInvalidArgumentCount)
{
	RunFailureTest("class X{implicit X(){}}", "Implicit constructor require single argument.");

	IType* type = module->AddType<A>("A");
	bool r = type->AddMethod("implicit A()", AsCtor<A>());
	Assert::IsFalse(r);
	AssertError("Implicit constructor require single argument.");
}

TEST_METHOD(InvalidImplicitMethod)
{
	RunFailureTest("class X{implicit void f(){}}", "Implicit can only be used for constructor and cast operators.");
	RunFailureTest("class X{implicit void operator += (int a){}}", "Implicit can only be used for constructor and cast operators.");
}

TEST_METHOD(CantCast)
{
	RunFailureTest("class A{} void f(int x){} A a; f(a as int);", "Can't cast from 'A' to 'int'.");
}

TEST_METHOD(ClassRedeclaration)
{
	RunFailureTest("class A{} class A{}", "Can't declare class 'A', type is already declared.");
}

TEST_METHOD(UndeclaredTypeUsed)
{
	RunFailureTest("A f(){}", "Undeclared type 'A' used.");
}

TEST_METHOD(MissingFunctionClosingBrace)
{
	RunFailureTest("void f(){", "Missing closing '}' for function 'f' declaration.");
}

TEST_METHOD(ReferenceAssignToInvalidType)
{
	RunFailureTest("int a; 3 -> a;", "Can't assign reference, left value must be reference variable.");
	RunFailureTest("int a; int& b -> a; b -> 3;", "Can't assign reference, right value must be variable.");
	RunFailureTest("int a; float b; float& c -> b; c -> a;", "Can't reference assign 'int' to type 'float&'.");
	RunFailureTest("int& a -> 3;", "Can't assign type 'int' to variable 'int& a'.");
}

TEST_METHOD(CallDeletedFunction)
{
	RunFailureTest("delete void f()();", "Can't call 'void f()', function marked as deleted.");
	RunFailureTest("class X{delete void f()} X x; x.f();", "Can't call 'void X.f()', method marked as deleted.");
	RunFailureTest("class X{delete bool operator == (X x)} X x; x == x;", "Can't call 'bool X.operator == (X)', method marked as deleted.");
}

TEST_METHOD(InvalidLongRefAssign)
{
	RunFailureTest("int a, b; a --> b;", "Can't long assign reference, left value must be reference to class.");
	RunFailureTest("class X{} void f(X& x) { x --> 3; }", "Can't long assign reference, right value must be variable.");
	RunFailureTest("class X{} class X2{} void f(X& x) { X2 x2; x --> x2; }", "Can't long reference assign 'X2' to type 'X&'.");
}

TEST_METHOD(IndexOutOfRange)
{
	RunFailureTest("string s; s[2] = 'c';", "Exception: Index 2 out of range.");
}

TEST_METHOD(IndexOutOfRangeOnReference)
{
	RunFailureTest(R"code(
		string s = "test";
		char& c = s[1];
		s.clear();
		c = 'd';
	)code", "Exception: Index 1 out of range.");
}

TEST_METHOD(DisallowCreateType)
{
	module->AddType<A>("A", cas::DisallowCreate);
	RunFailureTest("A a;", "Type 'A' cannot be created in script.");

	IType* type = module->AddType<B>("B", cas::DisallowCreate);
	type->AddMethod("B(int a, int b)", cas::AsCtor<B, int, int>());
	RunFailureTest("void f(B b){} f(B(1,2));", "Type 'B' cannot be created in script.");
}

CA_TEST_CLASS_END();
