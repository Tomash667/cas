#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

extern ostringstream s_output;

class A
{
public:
	void f(int) {}
	static void static_f(int, float) {}
	float f2(int, char) { return 0.f; }

	int var;
};

void f(char, float) {}

CA_TEST_CLASS(Reflection);

TEST_METHOD(GetFunctionByName)
{
	// register
	bool ok = module->AddFunction("void f(char,float)", f);
	Assert::IsTrue(ok);
	auto type = module->AddType<A>("A");
	Assert::IsNotNull(type);
	ok = type->AddMethod("void f(int x)", &A::f);
	Assert::IsTrue(ok);
	ok = type->AddMethod("static void static_f(int,float)", A::static_f);
	Assert::IsTrue(ok);
	auto result = module->Parse("class B{void f(string){} static void g(int,int,float c){}} void h(bool){}");
	Assert::AreEqual(cas::IModule::Ok, result);

	// verify
	IFunction* func = module->GetFunction("f");
	Assert::IsNotNull(func);

	IType* itype = module->GetType("A");
	Assert::IsNotNull(itype);
	
	func = itype->GetMethod("f");
	Assert::IsNotNull(func);

	func = itype->GetMethod("static_f");
	Assert::IsNotNull(func);

	func = module->GetFunction("h");
	Assert::IsNotNull(func);

	itype = module->GetType("B");
	Assert::IsNotNull(itype);

	func = itype->GetMethod("f");
	Assert::IsNotNull(func);

	func = itype->GetMethod("g");
	Assert::IsNotNull(func);

	func = module->GetFunction("print");
	Assert::IsNotNull(func);

	func = module->GetFunction("print", IgnoreParent);
	Assert::IsNull(func);
}

TEST_METHOD(GetFunctionByDecl)
{
	auto result = module->Parse("void f(int){} int f(int,float){return 0;} void f(char c,float fe,int i){}");
	Assert::AreEqual(cas::IModule::Ok, result);

	auto func = module->GetFunction("void f(int x)", ByDecl);
	Assert::IsNotNull(func);
	Assert::AreEqual("void f(int)", func->GetDecl());

	func = module->GetFunction("int   f ( int jada,   float bada ) ", ByDecl);
	Assert::IsNotNull(func);
	Assert::AreEqual("int f(int,float)", func->GetDecl());

	func = module->GetFunction("void  f (char,float,int) ", ByDecl);
	Assert::IsNotNull(func);
	Assert::AreEqual("void f(char,float,int)", func->GetDecl());

	func = module->GetFunction("void print(string&)", ByDecl);
	Assert::IsNotNull(func);

	func = module->GetFunction("void print(string&)", ByDecl | IgnoreParent);
	Assert::IsNull(func);
}

TEST_METHOD(GetFunctionsList)
{
	auto result = module->Parse("void f(int){} int f(int,float){return 0;} void f(char c,float fe,int i){} void g(){} void h(){}");
	Assert::AreEqual(cas::IModule::Ok, result);

	vector<IFunction*> funcs;
	module->GetFunctionsList(funcs, "f");
	Assert::AreEqual(3u, funcs.size());
	for(int i = 0; i < 3; ++i)
		Assert::AreEqual("f", funcs[i]->GetName());

	funcs.clear();
	module->GetFunctionsList(funcs, "println");
	Assert::AreEqual(2u, funcs.size());

	funcs.clear();
	module->GetFunctionsList(funcs, "println", IgnoreParent);
	Assert::AreEqual(0u, funcs.size());
}

TEST_METHOD(GetGlobal)
{
	auto result = module->Parse("int globox;");
	Assert::AreEqual(cas::IModule::Ok, result);

	auto global = module->GetGlobal("globox");
	Assert::IsNotNull(global);
	Assert::AreEqual("globox", global->GetName());
}

TEST_METHOD(GetType)
{
	auto result = module->Parse("class X{} enum E{}");
	Assert::AreEqual(cas::IModule::Ok, result);

	auto type = module->GetType("X");
	Assert::IsNotNull(type);

	type = module->GetType("E");
	Assert::IsNotNull(type);
}

TEST_METHOD(IFunctionOps)
{
	// register
	auto type = module->AddType<A>("A");
	Assert::IsNotNull(type);
	bool ok = type->AddMethod("void f(int)", &A::f);
	Assert::IsTrue(ok);
	auto result = module->Parse("string g(int a, float b=3){return a+b;}");
	Assert::AreEqual(cas::IModule::Ok, result);

	// get
	auto itype = module->GetType("A");
	Assert::IsNotNull(itype);
	auto func = itype->GetMethod("f");
	Assert::IsNotNull(func);
	auto func2 = module->GetFunction("g");
	Assert::IsNotNull(func2);

	// verify
	Assert::AreEqual(1u, func->GetArgCount());
	auto arg_type = func->GetArgType(0);
	Assert::AreEqual(GenericType::Int, arg_type.type->GetGenericType());
	auto value = func->GetArgDefaultValue(0);
	Assert::AreEqual(GenericType::Void, value.type->GetGenericType());
	auto clas = func->GetClass();
	Assert::IsTrue(clas == itype);
	Assert::AreEqual("void f(int)", func->GetDecl());
	Assert::AreEqual(IFunction::F_THISCALL | IFunction::F_CODE, func->GetFlags());
	Assert::IsTrue(module == func->GetModule());
	Assert::AreEqual("f", func->GetName());
	arg_type = func->GetReturnType();
	Assert::AreEqual(GenericType::Void, arg_type.type->GetGenericType());

	// verify2
	Assert::AreEqual(2u, func2->GetArgCount());
	arg_type = func2->GetArgType(0);
	Assert::AreEqual(GenericType::Int, arg_type.type->GetGenericType());
	arg_type = func2->GetArgType(1);
	Assert::AreEqual(GenericType::Float, arg_type.type->GetGenericType());
	value = func2->GetArgDefaultValue(0);
	Assert::AreEqual(GenericType::Void, value.type->GetGenericType());
	value = func2->GetArgDefaultValue(1);
	Assert::AreEqual(GenericType::Float, value.type->GetGenericType());
	Assert::AreEqual(3.f, value.float_value);
	clas = func2->GetClass();
	Assert::IsNull(clas);
	Assert::AreEqual("string g(int,float)", func2->GetDecl());
	Assert::AreEqual(0, func2->GetFlags());
	Assert::IsTrue(module == func2->GetModule());
	Assert::AreEqual("g", func2->GetName());
	arg_type = func2->GetReturnType();
	Assert::AreEqual(GenericType::String, arg_type.type->GetGenericType());
}

TEST_METHOD(GetEnumValues)
{
	// register
	auto type = module->AddEnum("A");
	Assert::IsNotNull(type);
	bool ok = type->AddValue("A1", 1);
	Assert::IsTrue(ok);
	ok = type->AddValue("A2");
	Assert::IsTrue(ok);
	ok = type->AddValue("A3", 5);
	Assert::IsTrue(ok);
	ok = type->AddValue("A4");
	Assert::IsTrue(ok);
	auto result = module->Parse("enum B{B1,B2=2,B3}");
	Assert::AreEqual(cas::IModule::Ok, result);

	// get
	auto itype = module->GetType("B");
	Assert::IsNotNull(itype);
	
	// verify
	auto& v = type->GetEnumValues();
	Assert::AreEqual(4u, v.size());
	Assert::AreEqual("A1", v[0].first.c_str());
	Assert::AreEqual(1, v[0].second);
	Assert::AreEqual("A2", v[1].first.c_str());
	Assert::AreEqual(2, v[1].second);
	Assert::AreEqual("A3", v[2].first.c_str());
	Assert::AreEqual(5, v[2].second);
	Assert::AreEqual("A4", v[3].first.c_str());
	Assert::AreEqual(6, v[3].second);

	// verify2
	auto& v2 = itype->GetEnumValues();
	Assert::AreEqual(3u, v2.size());
	Assert::AreEqual("B1", v2[0].first.c_str());
	Assert::AreEqual(0, v2[0].second);
	Assert::AreEqual("B2", v2[1].first.c_str());
	Assert::AreEqual(2, v2[1].second);
	Assert::AreEqual("B3", v2[2].first.c_str());
	Assert::AreEqual(3, v2[2].second);
}

TEST_METHOD(IMemberOps)
{
	// register
	auto type = module->AddType<A>("A");
	Assert::IsNotNull(type);
	bool ok = type->AddMember("int var", offsetof(A, var));
	Assert::IsTrue(ok);
	auto result = module->Parse("class B{float x, y;}");
	Assert::AreEqual(cas::IModule::Ok, result);

	// get
	auto itype = module->GetType("A");
	Assert::IsNotNull(itype);
	auto member = itype->GetMember("var");
	Assert::IsNotNull(member);
	auto itype2 = module->GetType("B");
	Assert::IsNotNull(itype2);
	auto member2 = itype2->GetMember("y");
	Assert::IsNotNull(member2);

	// verify
	Assert::IsTrue(itype == member->GetClass());
	Assert::IsTrue(module == member->GetModule());
	Assert::AreEqual("var", member->GetName());
	Assert::AreEqual(0u, member->GetOffset());
	auto var_type = member->GetType();
	Assert::AreEqual(GenericType::Int, var_type.type->GetGenericType());

	// verify2
	Assert::IsTrue(itype2 == member2->GetClass());
	Assert::IsTrue(module == member2->GetModule());
	Assert::AreEqual("y", member2->GetName());
	Assert::AreEqual(4u, member2->GetOffset());
	var_type = member2->GetType();
	Assert::AreEqual(GenericType::Float, var_type.type->GetGenericType());
}

TEST_METHOD(IGlobalOps)
{
	// register
	auto result = module->Parse("int globalx;");
	Assert::AreEqual(cas::IModule::Ok, result);

	// get
	auto global = module->GetGlobal("globalx");
	Assert::IsNotNull(global);

	// verify
	Assert::IsTrue(module == global->GetModule());
	Assert::AreEqual("globalx", global->GetName());
	auto var_type = global->GetType();
	Assert::AreEqual(GenericType::Int, var_type.type->GetGenericType());
}

TEST_METHOD(ITypeOps)
{
	// register
	auto type = module->AddType<A>("A");
	Assert::IsNotNull(type);
	bool ok = type->AddMethod("void f(int)", &A::f);
	Assert::IsTrue(ok);
	ok = type->AddMethod("float f(int,char)", &A::f2);
	Assert::IsTrue(ok);
	ok = type->AddMember("int var", offsetof(A, var));
	Assert::IsTrue(ok);

	// verify
	auto member = type->GetMember("var");
	Assert::IsNotNull(member);
	Assert::AreEqual("var", member->GetName());
	member = type->GetMember("var2");
	Assert::IsNull(member);
	auto method = type->GetMethod("f");
	Assert::IsNotNull(method);
	Assert::AreEqual("void f(int)", method->GetDecl());
	method = type->GetMethod("g");
	Assert::IsNull(method);
	method = type->GetMethod("float f(int,char)", ByDecl);
	Assert::IsNotNull(method);
	Assert::AreEqual("float f(int,char)", method->GetDecl());
	method = type->GetMethod("char g()", ByDecl);
	Assert::IsNull(method);
	method = type->GetMethod("int float double", ByDecl);
	Assert::IsNull(method);
	vector<IFunction*> funcs;
	type->GetMethodsList(funcs, "f");
	Assert::AreEqual(2u, funcs.size());
	Assert::AreEqual("void f(int)", funcs[0]->GetDecl());
	Assert::AreEqual("float f(int,char)", funcs[1]->GetDecl());
	Assert::IsTrue(module == type->GetModule());
	Assert::AreEqual("A", type->GetName());
	funcs.clear();
	type->QueryMethods([&](IFunction* f) { funcs.push_back(f); return true; });
	Assert::AreEqual(2u, funcs.size());
	Assert::AreEqual("void f(int)", funcs[0]->GetDecl());
	Assert::AreEqual("float f(int,char)", funcs[1]->GetDecl());
	vector<IMember*> members;
	type->QueryMembers([&](IMember* m) { members.push_back(m); return true; });
	Assert::AreEqual(1u, members.size());
	Assert::AreEqual("var", members[0]->GetName());
}

static void fA() { s_output << "A"; }
static void fB() { s_output << "B"; }
static void fC() { s_output << "C"; }
static void fD() { s_output << "D"; }
TEST_METHOD(MixedFunctions)
{
	bool ok = module->AddFunction("void A()", fA);
	Assert::IsTrue(ok);
	auto result = module->Parse("void a(){print(\"a\");}");
	Assert::AreEqual(IModule::Ok, result);
	ok = module->AddFunction("void B()", fB);
	Assert::IsTrue(ok);
	result = module->Parse("void b(){print(\"b\");}");
	Assert::AreEqual(IModule::Ok, result);
	auto module2 = engine->CreateModule(module);
	ok = module2->AddFunction("void C()", fC);
	Assert::IsTrue(ok);
	result = module2->Parse("void c(){print(\"c\");}");
	Assert::AreEqual(IModule::Ok, result);
	ok = module2->AddFunction("void D()", fD);
	Assert::IsTrue(ok);
	result = module2->Parse("void d(){print(\"d\");}");
	Assert::AreEqual(IModule::Ok, result);

	result = module2->Parse("A();a();B();b();C();c();D();d();");
	Assert::AreEqual(IModule::Ok, result);
	WriteDecompileOutput(module2);
	auto cc = module2->CreateCallContext();
	ok = cc->Run();
	module2->Release();
	Assert::IsTrue(ok);

	AssertOutput("AaBbCcDd");
}

CA_TEST_CLASS_END();
