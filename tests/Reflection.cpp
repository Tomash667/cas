#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

extern ostringstream s_output;

namespace reflection_tests
{

class A
{
public:
	void f(int) {}
	static void static_f(int, float) {}
	float f2(int, char) { return 0.f; }

	int var;
};

void f(char, float) {}

class B
{
public:
	float x, y;
	char c;

	B(float x, char c, float y) : x(x), y(y), c(c) {}
};

string global_str;

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
	Assert::AreEqual(GenericType::Int, arg_type.generic_type);
	auto value = func->GetArgDefaultValue(0);
	Assert::AreEqual(GenericType::Void, value.type.generic_type);
	auto clas = func->GetClass();
	Assert::IsTrue(clas == itype);
	Assert::AreEqual("void f(int)", func->GetDecl());
	Assert::AreEqual(IFunction::F_THISCALL | IFunction::F_CODE, func->GetFlags());
	Assert::IsTrue(module == func->GetModule());
	Assert::AreEqual("f", func->GetName());
	arg_type = func->GetReturnType();
	Assert::AreEqual(GenericType::Void, arg_type.generic_type);

	// verify2
	Assert::AreEqual(2u, func2->GetArgCount());
	arg_type = func2->GetArgType(0);
	Assert::AreEqual(GenericType::Int, arg_type.generic_type);
	arg_type = func2->GetArgType(1);
	Assert::AreEqual(GenericType::Float, arg_type.generic_type);
	value = func2->GetArgDefaultValue(0);
	Assert::AreEqual(GenericType::Void, value.type.generic_type);
	value = func2->GetArgDefaultValue(1);
	Assert::AreEqual(GenericType::Float, value.type.generic_type);
	Assert::AreEqual(3.f, value.float_value);
	clas = func2->GetClass();
	Assert::IsNull(clas);
	Assert::AreEqual("string g(int,float)", func2->GetDecl());
	Assert::AreEqual(0, func2->GetFlags());
	Assert::IsTrue(module == func2->GetModule());
	Assert::AreEqual("g", func2->GetName());
	arg_type = func2->GetReturnType();
	Assert::AreEqual(GenericType::String, arg_type.generic_type);
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
	Assert::AreEqual(GenericType::Int, var_type.generic_type);

	// verify2
	Assert::IsTrue(itype2 == member2->GetClass());
	Assert::IsTrue(module == member2->GetModule());
	Assert::AreEqual("y", member2->GetName());
	Assert::AreEqual(4u, member2->GetOffset());
	var_type = member2->GetType();
	Assert::AreEqual(GenericType::Float, var_type.generic_type);
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
	Assert::AreEqual(GenericType::Int, var_type.generic_type);
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

TEST_METHOD(SetEntryPoint)
{
	auto result = module->Parse("int suma(int x, int y) { return x+y; }");
	Assert::AreEqual(IModule::Ok, result);
	auto func = module->GetFunction("suma");
	auto context = module->CreateCallContext();
	bool ok = context->SetEntryPoint(func, 1, 2);
	Assert::IsTrue(ok);
	ok = context->Run();
	Assert::IsTrue(ok);
	auto ret = Retval(context);
	ret.IsInt(3);
	context->Release();
}

TEST_METHOD(SetEntryPointObj)
{
	auto result = module->Parse(R"code(
	struct INT2
	{
		int x,y;
		INT2(int _x, int _y)
		{
			x = _x;
			y = _y;
		}
		int suma()
		{
			return x+y;
		}
	}
	)code");
	Assert::AreEqual(IModule::Ok, result);

	auto type = module->GetType("INT2");
	auto func = type->GetMethod("suma");
	auto context = module->CreateCallContext();
	auto obj = context->CreateInstance(type, 3, 4);
	Assert::IsNotNull(obj);
	bool ok = context->SetEntryPointObj(func, obj);
	obj->Release();
	Assert::IsTrue(ok);
	ok = context->Run();
	Assert::IsTrue(ok);
	auto ret = Retval(context);
	ret.IsInt(7);	
	context->Release();
}

TEST_METHOD(CreateClassInstance)
{
	auto type = module->AddType<B>("B");
	Assert::IsNotNull(type);
	type->AddMember("float x", offsetof(B, x));
	type->AddMember("float y", offsetof(B, y));
	type->AddMember("char c", offsetof(B, c));
	bool ok = type->AddCtor<float, int, float>("B(float, char, float = 3.25)");
	Assert::IsTrue(ok);

	auto result = module->Parse("void f(B b) { print(\"\"+b.x+','+b.c+','+b.y); }");
	Assert::AreEqual(IModule::Ok, result);

	auto context = module->CreateCallContext();
	auto obj = context->CreateInstance(type, 4.5f, 'x');
	Assert::IsNotNull(obj);
	ok = context->SetEntryPoint(module->GetFunction("f"), obj);
	obj->Release();
	Assert::IsTrue(ok);

	ok = context->Run();
	Assert::IsTrue(ok);
	context->Release();

	AssertOutput("4.5,x,3.25");
}

TEST_METHOD(ReturnScriptClassToCaller)
{
	auto result = module->Parse(R"code(
		class C
		{
			int i;
			float f;
			char c;
		}

		C c;
		c.i = 3;
		c.f = 0.14;
		c.c = 'p';
		return c;
	)code");
	Assert::AreEqual(IModule::Ok, result);

	auto context = module->CreateCallContext();
	bool ok = context->Run();
	Assert::IsTrue(ok);
	auto ret = context->GetReturnValue();
	Assert::AreEqual(cas::GenericType::Class, ret.type.generic_type);
	auto obj = ret.obj;
	obj->AddRef();
	context->Release();
	Assert::AreEqual(cas::GenericType::Class, obj->GetType().generic_type);
	auto type = obj->GetType().specific_type;
	Assert::AreEqual("C", type->GetName());
	
	Assert::AreEqual(3, obj->GetMemberValue<int>(type->GetMember("i")));
	Assert::AreEqual(0.14f, obj->GetMemberValue<float>(type->GetMember("f")));
	Assert::AreEqual('p', obj->GetMemberValue<char>(type->GetMember("c")));
	obj->Release();
}

TEST_METHOD(ReturnCodeClassToCaller)
{
	auto type = module->AddType<B>("B");
	type->AddMember("float x", offsetof(B, x));
	type->AddMember("float y", offsetof(B, y));
	type->AddMember("char c", offsetof(B, c));

	auto result = module->Parse(R"code(
		B b;
		b.x = 1.24;
		b.y = 3.66;
		b.c = 'x';
		return b;
	)code");
	Assert::AreEqual(IModule::Ok, result);

	auto context = module->CreateCallContext();
	bool ok = context->Run();
	Assert::IsTrue(ok);

	auto ret = context->GetReturnValue();
	Assert::AreEqual(GenericType::Class, ret.type.generic_type);
	Assert::AreEqual("B", ret.type.specific_type->GetName());

	auto obj = ret.obj;
	obj->AddRef();
	context->Release();

	B& b = obj->Cast<B>();
	Assert::AreEqual(1.24f, b.x);
	Assert::AreEqual(3.66f, b.y);
	Assert::AreEqual('x', b.c);
	obj->Release();
}

TEST_METHOD(StringGlobalInScript)
{
	auto result = module->Parse(R"code(
		string x = "123";
		x += "456";
	)code");
	Assert::AreEqual(IModule::Ok, result);

	auto context = module->CreateCallContext();
	auto global = context->GetGlobal("x");
	Assert::IsNotNull(global);
	Assert::AreEqual("123", global->GetValueRef<string>().c_str());

	bool ok = context->Run();
	Assert::IsTrue(ok);
	Assert::AreEqual("123456", global->GetValueRef<string>().c_str());

	context->Release();
}

TEST_METHOD(StringGlobalInCode)
{
	bool ok = module->AddGlobal("string s", &global_str);
	Assert::IsTrue(ok);

	global_str = "pie";

	auto result = module->ParseAndRun(R"code(
		s += "ro";
		void f(string& str)
		{
			str += "gi";
		}
		f(s);
	)code");
	Assert::AreEqual(IModule::Ok, result);

	Assert::AreEqual("pierogi", global_str.c_str());
}

TEST_METHOD(PushStringValue)
{
	auto result = module->Parse(R"code(
		void f(string s)
		{
			println("f:"+s);
		}
		void f2(string& s)
		{
			s = "f2:"+s;
		}
	)code");
	Assert::AreEqual(IModule::Ok, result);

	auto context = module->CreateCallContext();

	auto fun = module->GetFunction("f");
	Assert::IsNotNull(fun);
	bool ok = context->SetEntryPoint(fun, "test");
	Assert::IsTrue(ok);
	context->Run();
	Assert::IsTrue(ok);
	AssertOutput("f:test\n");
	CleanupOutput();

	fun = module->GetFunction("f2");
	Assert::IsNotNull(fun);
	string str = "test2";
	ok = context->SetEntryPoint(fun, str);
	Assert::IsTrue(ok);
	ok = context->Run();
	Assert::IsTrue(ok);
	Assert::AreEqual("f2:test2", str.c_str());

	context->Release();
}

#define CHECK_OK(x) {bool ok = (x); Assert::IsTrue(ok, L"Operation failed", LINE_INFO());}
#define CHECK_OK_RESULT(x) {auto result = (x); Assert::AreEqual(IModule::Ok, result, L"Parse failed", LINE_INFO());}
#define CHECK_NULL(x) {Assert::IsNotNull(x, L"Value is null", LINE_INFO());}
#define EQUAL(x,y) {Assert::AreEqual(x, y, L"Equal failed", LINE_INFO());}

TEST_METHOD(PushEnumValue)
{
	enum class F
	{
		AA,
		BB,
		CC,
		DD
	};

	auto enu = module->AddEnum("F");
	CHECK_NULL(enu);
	CHECK_OK(enu->AddValues({ "AA","BB","CC", "DD" }));

	CHECK_OK_RESULT(module->Parse(R"code(
		enum E
		{
			AA,
			BB,
			CC
		}

		void fe(E e)
		{
			println("fe:"+e);
		}

		void fe2(E& e)
		{
			println("fe2:"+e);
			e = E.AA;
		}

		void ff(F f)
		{
			println("ff:"+f);
		}

		void ff2(F& f)
		{
			println("ff2:"+f);
			f = F.AA;
		}
	)code"));

	WriteDecompileOutput(module);

	// pass script enum by value
	auto context = module->CreateCallContext();
	auto fun = module->GetFunction("fe");
	CHECK_NULL(fun);
	auto s_enu = module->GetType("E");
	CHECK_OK(context->SetEntryPoint(fun, Value::Enum(s_enu, 1)));
	CHECK_OK(context->Run());
	AssertOutput("fe:1\n");
	CleanupOutput();

	// pass script enum by ref
	fun = module->GetFunction("fe2");
	CHECK_NULL(fun);
	int val = 2;
	CHECK_OK(context->SetEntryPoint(fun, Value::Enum(s_enu, &val)));
	CHECK_OK(context->Run());
	AssertOutput("fe2:2\n");
	CleanupOutput();
	EQUAL(0, val);

	// pass code enum by val
	fun = module->GetFunction("ff");
	CHECK_NULL(fun);
	CHECK_OK(context->SetEntryPoint(fun, Value::Enum(enu, F::CC)));
	CHECK_OK(context->Run());
	AssertOutput("ff:2\n");
	CleanupOutput();

	// pass code enum by ref
	F f = F::DD;
	fun = module->GetFunction("ff2");
	CHECK_NULL(fun);
	CHECK_OK(context->SetEntryPoint(fun, Value::Enum(enu, &f)));
	CHECK_OK(context->Run());
	AssertOutput("ff2:3\n");
	CleanupOutput();
	EQUAL((int)F::AA, (int)f);

	context->Release();
}

TEST_METHOD(EnumGlobal)
{
	enum class E
	{
		AA,
		BB,
		CC
	};
	auto enu = module->AddEnum("E");
	CHECK_NULL(enu);
	CHECK_OK(enu->AddValues({ "AA","BB","CC" }));
	E e = E::BB;
	CHECK_OK(module->AddGlobal("E e", &e));

	CHECK_OK_RESULT(module->Parse(R"code(
		E e2 = E.CC;
		e = E.AA;
		e2 = E.BB;
	)code"));

	auto context = module->CreateCallContext();
	auto cglobal = context->GetGlobal("e");
	CHECK_NULL(cglobal);
	CHECK_OK(cglobal->GetEnum<E>() == e);
	auto sglobal = context->GetGlobal("e2");
	CHECK_NULL(sglobal);
	CHECK_OK(sglobal->GetEnum<E>() == E::CC);

	CHECK_OK(context->Run());
	CHECK_OK(cglobal->GetEnum<E>() == e);
	CHECK_OK(E::AA == e);
	CHECK_OK(sglobal->GetEnum<E>() == E::BB);
	context->Release();
}

CA_TEST_CLASS_END();

}
