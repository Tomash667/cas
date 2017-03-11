#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

class A
{
public:
	void f(int) {}
	static void static_f(int, float) {}
};

void f(char, float) {}

CA_TEST_CLASS(Reflection);

TEST_METHOD(GetFunctionByName)
{
	// register
	bool ok = module->AddFunction("void f(char c, float g)", f);
	Assert::IsTrue(ok);
	auto type = module->AddType<A>("A");
	Assert::IsNotNull(type);
	ok = type->AddMethod("void f(int x)", &A::f);
	Assert::IsTrue(ok);
	ok = type->AddMethod("static void static_f(int i, float g)", A::static_f);
	Assert::IsTrue(ok);

	// verify
	IFunction* func = module->GetFunction("f");
	Assert::IsNotNull(func);

	IType* itype = module->GetType("A");
	Assert::IsNotNull(itype);
	
	func = itype->GetMethod("f");
	Assert::IsNotNull(func);

	func = itype->GetMethod("static_f");
	Assert::IsNotNull(func);
}

CA_TEST_CLASS_END();
