#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class A
{
public:
	void f() {}
	void f2(int) {}
	void f2(string&) {}

	int x, y;
};

class B
{
public:
	B(int a, int b) {}
};

class C : public RefCounter
{
public:
};

static void f() {}
static A* A_ctor() { return new A; }
static A invalid_ctor() { return A(); }

CA_TEST_CLASS(Failures);

TEST_METHOD(FunctionNoReturnValue)
{
	RunTest("int f(){}")
		.ShouldFail("Function 'int f()' not always return value.");
}

TEST_METHOD(ItemNameAlreadyUsed)
{
	RunTest("int f, f;")
		.ShouldFail("Name 'f' already used as global variable.");
	RunTest("void f(int a) { int a; }")
		.ShouldFail("Name 'a' already used as argument.");
	RunTest("void f() { int a, a; }")
		.ShouldFail("Name 'a' already used as local variable.");
}

TEST_METHOD(MemberNameAlreadyUsed)
{
	RunTest("class A { int a, a; }")
		.ShouldFail("Member with name 'A.a' already exists.");
}

TEST_METHOD(NoMatchingCallToFunction)
{
	RunTest("void f(){} f(1);")
		.ShouldFail("No matching call to function 'f' with arguments (int), could be 'void f()'.");
}

TEST_METHOD(NoMatchingCallToMethod)
{
	RunTest("class A { void f(){} } A a; a.f(1);")
		.ShouldFail("No matching call to method 'A.f' with arguments (int), could be 'void A.f()'.");
}

TEST_METHOD(AmbiguousCallToFunction)
{
	RunTest("void f(bool b){} void f(float g){} f(1);")
		.ShouldFail("Ambiguous call to overloaded function 'f' with arguments (int), could be:\n\tvoid f(bool)\n\tvoid f(float)");
}

TEST_METHOD(AmbiguousCallToMethod)
{
	RunTest("class A { void f(bool b){} void f(float g){} } A a; a.f(1);")
		.ShouldFail("Ambiguous call to overloaded method 'A.f' with arguments (int), could be:\n\tvoid A.f(bool)\n\tvoid A.f(float)");
}

TEST_METHOD(MissingConstructor)
{
	RunTest("class A {}  A a = A(1,2,3);")
		.ShouldFail("No matching call to method 'A.A' with arguments (int,int,int), could be 'A.A()'.");
}

TEST_METHOD(MissingDefaultValue)
{
	RunTest("void f(int a=0,float b){}")
		.ShouldFail("Missing default value for argument 2 'float b'.");
}

TEST_METHOD(InvalidAssignType)
{
	RunTest("class A{} A a; int b = a;")
		.ShouldFail("Can't assign type 'A' to variable 'int b'.");
}

TEST_METHOD(InvalidConditionType)
{
	RunTest("class A{} A a; if(a) {}")
		.ShouldFail("Condition expression with 'A' type.");
	RunTest("class A{} A a; for(;a;) {}")
		.ShouldFail("Condition expression with 'A' type.");
}

TEST_METHOD(VoidVariable)
{
	RunTest("void a;")
		.ShouldFail("Can't declare void variable.");
	RunTest("class A{void a;}")
		.ShouldFail("Member of 'void' type not allowed.");
}

TEST_METHOD(InvalidDefaultValue)
{
	RunTest("void f(int a=\"dodo\"){}")
		.ShouldFail("Invalid default value of type 'string', required 'int'.");
}

TEST_METHOD(UnsupportedClassMembers)
{
	RunTest("class A{} class B{A a;}")
		.ShouldFail("Member of 'class' type not allowed yet.");
	RunTest("struct A{} class B{A a;}")
		.ShouldFail("Member of 'struct' type not allowed yet.");
}

TEST_METHOD(InvalidBreak)
{
	RunTest("break;")
		.ShouldFail("Not in breakable block.");
}

TEST_METHOD(InvalidReturnType)
{
	RunTest("int f() { return \"dodo\"; }")
		.ShouldFail("Invalid return type 'string', function 'int f()' require 'int' type.");
}

TEST_METHOD(InvalidClassDeclaration)
{
	RunTest("void f() { class A{} }")
		.ShouldFail("Class can't be declared inside block.");
}

TEST_METHOD(InvalidFunctionDeclaration)
{
	RunTest("void f() { void g() {} }")
		.ShouldFail("Function can't be declared inside block.");
	RunTest("{ void g() {} }")
		.ShouldFail("Function can't be declared inside block.");
}

TEST_METHOD(FunctionRedeclaration)
{
	RunTest("void f(){} void f(){}")
		.ShouldFail("Function 'void f()' already exists.");
	RunTest("class A{ void f(){} void f(){} }")
		.ShouldFail("Method 'void A.f()' already exists.");
}

TEST_METHOD(FunctionRedeclarationComplex)
{
	bool ok = module->AddFunction("void f()", f);
	Assert::IsTrue(ok);
	auto result = module->Parse("void g() {}");
	Assert::AreEqual(IModule::Ok, result);

	ok = module->AddFunction("void g()", f);
	Assert::IsFalse(ok);
	AssertError("Function 'void g()' already exists.");
	CleanupErrors();

	IModule* module2 = engine->CreateModule(module);
	ok = module2->AddFunction("void g()", f);
	Assert::IsFalse(ok);
	AssertError("Function 'void g()' already exists.");
	CleanupErrors();

	result = module2->Parse("void f(){}");
	Assert::AreEqual(IModule::ParsingError, result);
	AssertError("Function 'void f()' already exists.");
	module2->Release();
}

TEST_METHOD(ReferenceVariableUninitialized)
{
	RunTest("int& a;")
		.ShouldFail("Uninitialized reference variable.");
}

TEST_METHOD(InvalidOperationTypes)
{
	RunTest("void f(){} 3+f();")
		.ShouldFail("Invalid types 'int' and 'void' for operation 'add'.");
	RunTest("void f(){} -f();")
		.ShouldFail("Invalid type 'void' for operation 'unary minus'.");
	RunTest("bool b; ++b;")
		.ShouldFail("Invalid type 'bool' for operation 'pre increment'.");
	RunTest("int a; a+=\"dodo\";")
		.ShouldFail("Can't cast return value from 'string' to 'int' for operation 'assign add'.");
	RunTest("void f(int& a){a+=\"dodo\";}")
		.ShouldFail("Can't cast return value from 'string' to 'int' for operation 'assign add'.");
}

TEST_METHOD(RequiredVariable)
{
	RunTest("++3;")
		.ShouldFail("Operation 'pre increment' require variable.");
	RunTest("3 = 1;")
		.ShouldFail("Can't assign, left must be assignable.");
}

TEST_METHOD(InvalidMemberAccess)
{
	RunTest("void f(){} f().dodo();")
		.ShouldFail("Missing method 'dodo' for type 'void'.");
	RunTest("3.ok();")
		.ShouldFail("Missing method 'ok' for type 'int'.");
	RunTest("class A{} A a; a.ok();")
		.ShouldFail("Missing method 'ok' for type 'A'.");
	RunTest("class A{} A a; a.b=1;")
		.ShouldFail("Missing member 'b' for type 'A'.");
}

TEST_METHOD(InvalidAssignTypes)
{
	RunTest("int a; a=\"dodo\";")
		.ShouldFail("No matching call to method 'int.operator =' with arguments (string), could be 'int int.operator = (int)'.");
	RunTest("void f(int& a) {a=\"dodo\";}")
		.ShouldFail("No matching call to method 'int.operator =' with arguments (string), could be 'int int.operator = (int)'.");
}

TEST_METHOD(InvalidGlobalReturnType)
{
	RunTest("int& f(int& a){return a;} int a; return f(a);")
		.ShouldFail("Invalid type 'int&' for global return.");
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
	IClass* type = module->AddType<A>("A");
	Assert::IsNotNull(type);

	bool r = type->AddMethod("void f;", &A::f);
	Assert::IsFalse(r);
	AssertError("Expecting symbol '(', found symbol ';'.");
	AssertError("Failed to parse function declaration for AddMethod 'void f;'.");
}

TEST_METHOD(RegisterSameMethodTwice)
{
	IClass* type = module->AddType<A>("A");
	Assert::IsNotNull(type);

	bool r = type->AddMethod("void f()", &A::f);
	Assert::IsTrue(r);

	r = type->AddMethod("void f()", &A::f);
	Assert::IsFalse(r);
	AssertError("Method 'void f()' for type 'A' already exists.");
}

TEST_METHOD(RegisterMemberParseError)
{
	IClass* type = module->AddType<A>("A");
	Assert::IsNotNull(type);

	bool r = type->AddMember("int;", offsetof(A, x));
	Assert::IsFalse(r);
	AssertError("Expecting item, found symbol ';'.");
	AssertError("Failed to parse member declaration for type 'A' AddMember 'int;'.");
}

TEST_METHOD(InvalidSubscriptIndexType)
{
	RunTest("string a; a[a] = 'b';")
		.ShouldFail("No matching call to method 'string.operator []' with arguments (string), could be 'char& string.operator [] (int)'.");
}

TEST_METHOD(InvalidSubscriptType)
{
	RunTest("int a; a[0] = 1;")
		.ShouldFail("Type 'int' don't have subscript operator.");
}

TEST_METHOD(OperatorFunctionOutsideClass)
{
	RunTest("void operator += (int a) {}")
		.ShouldFail("Operator function can be used only inside class.");
}

TEST_METHOD(CantOverloadOperator)
{
	RunTest("class A{void operator . (){}}")
		.ShouldFail("Can't overload operator '.'.");
}

TEST_METHOD(InvalidOperatorOverloadDefinition)
{
	RunTest("class A{void operator + (int a, int b){}}")
		.ShouldFail("Invalid overload operator definition 'void A.operator + (int,int)'.");
}

TEST_METHOD(AmbiguousCallToOverloadedOperator)
{
	RunTest(R"code(
		class A{
			void operator += (int a) {}
			void operator += (string s) {}
		}
		A a;
		a += 1.13;
	)code")
		.ShouldFail("Ambiguous call to overloaded method 'A.operator +=' with arguments (float), could be:\n\tvoid A.operator += (int)\n\tvoid A.operator += (string)");

	auto type = module->AddType<A>("A");
	type->AddMethod("void f(int i)", AsMethod(A, f2, void, (int)));
	type->AddMethod("void f(string& s)", AsMethod(A, f2, void, (string&)));
	RunTest("A a; a.f(3.14);")
		.ShouldFail("Ambiguous call to overloaded method 'A.f' with arguments (float), could be:\n\tvoid A.f(int)\n\tvoid A.f(string&)");
}

TEST_METHOD(InvalidFunctorArgs)
{
	RunTest("class A{ void operator () (int a) {}} A a; a(3, 14);")
		.ShouldFail("No matching call to method 'A.operator ()' with arguments (int,int), could be 'void A.operator () (int)'.");
}

TEST_METHOD(InvalidFunctorType)
{
	RunTest("int a = 4; a();")
		.ShouldFail("Type 'int' don't have call operator.");
}

TEST_METHOD(InvalidTernaryCommonType)
{
	RunTest("void f(){} 1?f():1;")
		.ShouldFail("Invalid common type for ternary operator with types 'void' and 'int'.");
}

TEST_METHOD(InvalidTernaryCondition)
{
	RunTest("void f(){} f()?0:1;")
		.ShouldFail("Ternary condition expression with 'void' type.");
}

TEST_METHOD(InvalidSwitchType)
{
	RunTest("class X{} X x; switch(x){}")
		.ShouldFail("Invalid switch type 'X'.");
}

TEST_METHOD(CantCastCaseType)
{
	RunTest("int a; switch(a){case \"a\":}")
		.ShouldFail("Can't cast case value from 'string' to 'int'.");
}

TEST_METHOD(CaseAlreadyDefined)
{
	RunTest("int a; switch(a){case 0:case 0:}")
		.ShouldFail("Case with value '0' is already defined.");
}

TEST_METHOD(DefaultCaseAlreadyDefined)
{
	RunTest("int a; switch(a){default:default:}")
		.ShouldFail("Default case already defined.");
}

TEST_METHOD(BrokenSwitch)
{
	RunTest("int a; switch(a){do}")
		.ShouldFail("Expecting keyword 'case' from group 'keywords', keyword 'default' from group 'keywords', "
			"found keyword 'do' from group 'keywords'.");
}

TEST_METHOD(ImplicitFunction)
{
	RunTest("implicit void f(){}")
		.ShouldFail("Implicit can only be used for methods.");
}

TEST_METHOD(ImplicitInvalidArgumentCount)
{
	RunTest("class X{implicit X(){}}")
		.ShouldFail("Implicit constructor require single argument.");

	auto type = module->AddType<A>("A");
	bool r = type->AddCtor("implicit A()");
	Assert::IsFalse(r);
	AssertError("Implicit constructor require single argument.");
}

TEST_METHOD(InvalidImplicitMethod)
{
	RunTest("class X{implicit void f(){}}")
		.ShouldFail("Implicit can only be used for constructor and cast operators.");
	RunTest("class X{implicit void operator += (int a){}}")
		.ShouldFail("Implicit can only be used for constructor and cast operators.");
}

TEST_METHOD(CantCast)
{
	RunTest("class A{} void f(int x){} A a; f(a as int);")
		.ShouldFail("Can't cast from 'A' to 'int'.");
}

TEST_METHOD(ClassRedeclaration)
{
	RunTest("class A{} class A{}")
		.ShouldFail("Can't declare class 'A', type is already declared.");
}

TEST_METHOD(UndeclaredTypeUsed)
{
	RunTest("A f(){A a; return a;}")
		.ShouldFail("Undeclared type 'A' used.");
}

TEST_METHOD(MissingFunctionClosingBrace)
{
	RunTest("void f(){")
		.ShouldFail("Missing closing '}' for function 'f' declaration.");
}

TEST_METHOD(ReferenceAssignToInvalidType)
{
	RunTest("int a; 3 -> a;")
		.ShouldFail("Can't assign reference, left value must be reference variable.");
	RunTest("int a; int& b -> a; b -> 3;")
		.ShouldFail("Can't assign reference, right value must be variable.");
	RunTest("int a; float b; float& c -> b; c -> a;")
		.ShouldFail("Can't reference assign 'int' to type 'float&'.");
	RunTest("int& a -> 3;")
		.ShouldFail("Can't assign type 'int' to variable 'int& a'.");
}

TEST_METHOD(CallDeletedFunction)
{
	RunTest("delete void f()();")
		.ShouldFail("Can't call 'void f()', function marked as deleted.");
	RunTest("class X{delete void f()} X x; x.f();")
		.ShouldFail("Can't call 'void X.f()', method marked as deleted.");
	RunTest("class X{delete bool operator == (X x)} X x; x == x;")
		.ShouldFail("Can't call 'bool X.operator == (X)', method marked as deleted.");
}

TEST_METHOD(InvalidLongRefAssign)
{
	RunTest("int a, b; a --> b;")
		.ShouldFail("Can't long assign reference, left value must be reference to class.");
	RunTest("class X{} void f(X& x) { x --> 3; }")
		.ShouldFail("Can't long assign reference, right value must be variable.");
	RunTest("class X{} class X2{} void f(X& x) { X2 x2; x --> x2; }")
		.ShouldFail("Can't long reference assign 'X2' to type 'X&'.");
}

TEST_METHOD(IndexOutOfRange)
{
	RunTest("string s; s[2] = 'c';")
		.ShouldFail("Exception: Index 2 out of range.");
}

TEST_METHOD(IndexOutOfRangeOnReference)
{
	RunTest(R"code(
		string s = "test";
		char& c = s[1];
		s.clear();
		c = 'd';
	)code")
		.ShouldFail("Exception: Index 1 out of range.");
}

TEST_METHOD(DisallowCreateType)
{
	module->AddType<A>("A", cas::DisallowCreate);
	RunTest("A aa;")
		.ShouldFail("Type 'A' cannot be created in script.");

	auto type = module->AddType<B>("B", cas::DisallowCreate);
	type->AddCtor<int, int>("B(int a, int b)");
	RunTest("void f(B b){} f(B(1,2));")
		.ShouldFail("Type 'B' cannot be created in script.");
}

struct X
{
	X(int x, int y) {}
	X() {}
};
TEST_METHOD(CodeCtorNotMatching)
{
	auto type = module->AddType<X>("A");
	type->AddCtor<int, int>("A(int x, int y)");
	RunTest("A a = A(3);")
		.ShouldFail("No matching call to method 'A.A' with arguments (int), could be 'A.A(int,int)'.");

	type = module->AddType<X>("B");
	type->AddCtor("B()");
	type->AddCtor<int, int>("B(int x, int y)");
	RunTest("B b = B(3);")
		.ShouldFail("Ambiguous call to overloaded method 'B.B' with arguments (int), could be:\n\tB.B()\n\tB.B(int,int)");
}

static void pass_code_class_by_value(X x) {}
TEST_METHOD(PassCodeClassByValue)
{
	module->AddType<X>("X");
	bool r = module->AddFunction("void f(X x)", pass_code_class_by_value);
	Assert::IsFalse(r);
	AssertError("Reference type 'X' must be passed by reference/pointer.");
	AssertError("Failed to parse function declaration for AddFunction 'void f(X x)'.");
}

static X create_x_by_value() { X x; return x; }
static X* create_x_by_pointer() { static X x; return &x; }
TEST_METHOD(CodeCtorInvalidReturnType)
{
	IClass* type = module->AddType<X>("X");
	bool r = type->AddMethod("X()", create_x_by_value);
	Assert::IsFalse(r);
	AssertError("Class constructor 'X()' must return type by reference/pointer.");
	CleanupErrors();

	type = module->AddType<X>("X2", cas::ValueType);
	r = type->AddMethod("X2()", create_x_by_pointer);
	Assert::IsFalse(r);
	AssertError("Struct constructor 'X2()' must return type by value.");
}

TEST_METHOD(ReturnCodeClassByValue)
{
	IClass* type = module->AddType<X>("X");
	bool r = type->AddMethod("X f()", create_x_by_value);
	Assert::IsFalse(r);
	AssertError("Class in code method must be returned by reference/pointer.");
	AssertError("Failed to parse function declaration for AddMethod 'X f()'.");
	CleanupErrors();

	r = type->AddMethod("X()", create_x_by_value);
	Assert::IsFalse(r);
	AssertError("Class constructor 'X()' must return type by reference/pointer.");
}

TEST_METHOD(StaticOnFunction)
{
	RunTest("static void f(){}")
		.ShouldFail("Static can only be used for methods.");
}

TEST_METHOD(MultipleSameFunctionModifiers)
{
	RunTest("delete delete void f(){}")
		.ShouldFail("Delete already declared for this function.");
	RunTest("class X{delete delete void f(){}}")
		.ShouldFail("Delete already declared for this method.");
	RunTest("class X{implicit implicit X(){}}")
		.ShouldFail("Implicit already declared for this method.");
	RunTest("class X{static static void f(){}}")
		.ShouldFail("Static already declared for this method.");

	auto type = module->AddType<A>("A");
	bool r = type->AddMethod("static static void f()", f);
	Assert::IsFalse(r);
	AssertError("Static already declared for this method.");
	AssertError("Failed to parse function declaration for AddMethod 'static static void f()'.");
	CleanupErrors();

	r = type->AddMethod("delete delete void f()", f);
	Assert::IsFalse(r);
	AssertError("Delete already declared for this method.");
	AssertError("Failed to parse function declaration for AddMethod 'delete delete void f()'.");
	CleanupErrors();

	r = type->AddMethod("implicit implicit int operator cast()", f);
	Assert::IsFalse(r);
	AssertError("Implicit already declared for this method.");
	AssertError("Failed to parse function declaration for AddMethod 'implicit implicit int operator cast()'.");
}

TEST_METHOD(StaticCtor)
{
	RunTest("class A{static A(){}}")
		.ShouldFail("Static constructor not allowed.");

	auto type = module->AddType<A>("A2");
	bool r = type->AddCtor("static A2()");
	Assert::IsFalse(r);
	AssertError("Static constructor not allowed.");
}

TEST_METHOD(StaticOperator)
{
	RunTest("class A{static bool operator == (int x){return false;}}")
		.ShouldFail("Static operator not allowed.");

	auto type = module->AddType<A>("A");
	bool r = type->AddMethod("static bool operator == (int x)", f);
	Assert::IsFalse(r);
	AssertError("Static operator not allowed.");
}

TEST_METHOD(MismatchedStaticMethod)
{
	RunTest("class A{static int sum(int x, int y){return x+y;}}A.sum(1,2,3);")
		.ShouldFail("No matching call to method 'A.sum' with arguments (int,int,int), could be 'int A.sum(int,int)'.");

	RunTest("class A{} A.sum(1,2,3);")
		.ShouldFail("Missing static method 'sum' for type 'A'.");
}

TEST_METHOD(RegisterEnumWithSameEnumerator)
{
	RunTest("enum E{A,A}")
		.ShouldFail("Enumerator 'E.A' already defined.");

	IEnum* enu = module->AddEnum("F");
	bool r = enu->AddValue("A");
	Assert::IsTrue(r);
	r = enu->AddValue("A", 1);
	Assert::IsFalse(r);
	AssertError("Enumerator 'F.A' already defined.");
	CleanupErrors();

	r = enu->AddValues({ "B", "B" });
	Assert::IsFalse(r);
	AssertError("Enumerator 'F.B' already defined.");
	CleanupErrors();

	r = enu->AddValues({ {"C", 2}, {"C", 4} });
	AssertError("Enumerator 'F.C' already defined.");
	Assert::IsFalse(r);
}

TEST_METHOD(KeywordAsEnumerator)
{
	RunTest("enum E{A,int}")
		.ShouldFail("Enumerator name 'int' already used as type.");

	IEnum* enu = module->AddEnum("F");
	bool r = enu->AddValue("int");
	Assert::IsFalse(r);
	AssertError("Enumerator name 'int' already used as type.");
}

TEST_METHOD(EnumInsideBlock)
{
	RunTest("{enum E{}}")
		.ShouldFail("Enum can't be declared inside block.");
	RunTest("void f(){enum E{}}")
		.ShouldFail("Enum can't be declared inside block.");
}

TEST_METHOD(EnumInsideClass)
{
	RunTest("class C{enum E{}}")
		.ShouldFail("Enum can't be declared inside class.");
}

TEST_METHOD(ExpectingEnumName)
{
	RunTest("enum 4{}")
		.ShouldFail("Expecting enum name, found integer 4.");
}

TEST_METHOD(InvalidEnumValue)
{
	RunTest("enum E{A=\"1\"}")
		.ShouldFail("Enumerator require 'int' value, have 'string'.");
}

TEST_METHOD(UnclosedMultilineComment)
{
	RunTest("*/")
		.ShouldFail("Unclosed multiline comment.");
}

TEST_METHOD(DestructorWithArguments)
{
	RunTest("class A{~A(int x){}}")
		.ShouldFail("Destructor can't have arguments.");

	auto type = module->AddType<A>("A");
	bool result = type->AddMethod("~A(int x)", f);
	Assert::IsFalse(result);
	AssertError("Destructor can't have arguments.");
}

TEST_METHOD(RefCountedTypeDestructor)
{
	auto type = module->AddRefType<C>("C");
	bool result = type->AddMethod("~C()", f);
	Assert::IsFalse(result);
	AssertError("Reference counted type can't have destructor.");
}

TEST_METHOD(AlreadyDeclaredDestructor)
{
	RunTest("class A{~A(){} ~A(){}}")
		.ShouldFail("Type 'A' have already declared destructor.");

	auto type = module->AddType<A>("A");
	bool result = type->AddMethod("~A()", f);
	Assert::IsTrue(result);
	result = type->AddMethod("~A()", f);
	Assert::IsFalse(result);
	AssertError("Type 'A' have already declared destructor.");
}

TEST_METHOD(SimpleTypeCtorInvalidArgCount)
{
	RunTest("int a(4,5);")
		.ShouldFail("Constructor for type 'int' require single argument.");
	RunTest("class A{int a(4,5);}")
		.ShouldFail("Constructor for type 'int' require single argument.");
}

TEST_METHOD(ConstructorDestructorOutsideClass)
{
	RunTest("class A{} A(){}")
		.ShouldFail("Constructor can only be declared inside class.");
	RunTest("class A{} ~A(){}")
		.ShouldFail("Destructor can only be declared inside class.");

	auto type = module->AddType<A>("A");
	bool result = module->AddFunction("A()", A_ctor);
	Assert::IsFalse(result);
	AssertError("Constructor can only be declared inside class.");
	CleanupErrors();

	result = module->AddFunction("~A()", A_ctor);
	Assert::IsFalse(result);
	AssertError("Destructor can only be declared inside class.");
}

TEST_METHOD(StackOverflow)
{
	RunTest("void f(){f();}();")
		.ShouldFail("Exception: Stack overflow.");
}

TEST_METHOD(RedeclareFunctionWithPassByRef)
{
	module->AddFunction("void f(string& s)", f);
	RunTest("void f(string& s){}")
		.ShouldFail("Function 'void f(string&)' already exists.");
}

TEST_METHOD(DisallowGlobalCode)
{
	auto options = module->GetOptions();
	options.disallow_global_code = true;
	module->SetOptions(options);

	RunTest("for(int i=0; i<3; ++i){}")
		.ShouldFail("Global code disallowed.");

	options.disallow_global_code = false;
	module->SetOptions(options);
}

TEST_METHOD(DisallowGlobals)
{
	auto options = module->GetOptions();
	options.disallow_globals = true;
	module->SetOptions(options);

	RunTest("int a = 3;")
		.ShouldFail("Global variables disallowed.");

	options.disallow_globals = false;
	module->SetOptions(options);
}

CA_TEST_CLASS_END();
