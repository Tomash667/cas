#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

extern ostringstream s_output;

CA_TEST_CLASS(Code);

//=========================================================================================
struct INT2
{
	int x, y;

	inline INT2() {}
	inline explicit INT2(int v) : x(v), y(v) {}
	inline INT2(int x, int y) : x(x), y(y) {}
	template<typename T, typename T2>
	inline INT2(T x, T2 y) : x((int)x), y((int)y) {}
	inline INT2(const INT2& v) : x(v.x), y(v.y) {}

	inline INT2& operator = (const INT2& v)
	{
		x = v.x;
		y = v.y;
		return *this;
	}
};

static INT2* f_int2c_ctor0()
{
	return new INT2(0, 0);
}

static INT2* f_int2c_ctor1(int xy)
{
	return new INT2(xy);
}

static INT2* f_int2c_ctor2(int x, int y)
{
	return new INT2(x, y);
}

static void f_wypisz_int2c(INT2& i)
{
	s_output << "x:" << i.x << " y:" << i.y << "\n";
}

struct Vec2
{
	float x, y;
};

static Vec2* f_create_vec2(float x, float y)
{
	Vec2* v = new Vec2;
	v->x = x;
	v->y = y;
	return v;
}

static void f_wypisz_vec2(Vec2& v)
{
	s_output << "x:" << v.x << " y:" << v.y << "\n";
}

struct Vec3
{
	float x, y, z;
};

static Vec3* f_create_vec3(float x, float y, float z)
{
	Vec3* v = new Vec3;
	v->x = x;
	v->y = y;
	v->z = z;
	return v;
}

static void f_wypisz_vec3(Vec3& v)
{
	s_output << "x:" << v.x << " y:" << v.y << " z:" << v.z << "\n";
}

TEST_METHOD(ComplexClassResult)
{
	IClass* type;
	// Vec2
	type = module->AddType<Vec2>("Vec2");
	type->AddMember("float x", offsetof(Vec2, x));
	type->AddMember("float y", offsetof(Vec2, y));
	module->AddFunction("Vec2& create_vec2(float x, float y)", f_create_vec2);
	module->AddFunction("void wypisz_vec2(Vec2& v)", f_wypisz_vec2);
	// Vec3 (pod > 8 byte)
	type = module->AddType<Vec3>("Vec3");
	type->AddMember("float x", offsetof(Vec3, x));
	type->AddMember("float y", offsetof(Vec3, y));
	type->AddMember("float z", offsetof(Vec3, z));
	module->AddFunction("Vec3& create_vec3(float x, float y, float z)", f_create_vec3);
	module->AddFunction("void wypisz_vec3(Vec3& v)", f_wypisz_vec3);
	// INT2c have ctor
	type = module->AddType<INT2>("INT2c");
	type->AddMember("int x", offsetof(INT2, x));
	type->AddMember("int y", offsetof(INT2, y));
	type->AddMethod("INT2c()", f_int2c_ctor0);
	type->AddMethod("INT2c(int xy)", f_int2c_ctor1);
	type->AddMethod("INT2c(int x, int y)", f_int2c_ctor2);
	module->AddFunction("void wypisz_int2c(INT2c& i)", f_wypisz_int2c);

	RunTest(R"CODE(
		Vec3 v = create_vec3(1.5,2.3,4.1);
		v.x += 0.1;
		wypisz_vec3(v);
		Vec3 w = v;
		w.y += 0.1;
		wypisz_vec3(v);

		wypisz_int2c(INT2c());
		wypisz_int2c(INT2c(13));
		wypisz_int2c(INT2c(7,3));
		wypisz_vec2(create_vec2(3.14,0.0015));
	)CODE");
}

//=========================================================================================
struct Vec
{
	int x;

	Vec& operator += (int a)
	{
		x += a;
		return *this;
	}
};

TEST_METHOD(CodeRegisterValueType)
{
	IClass* type = module->AddType<Vec>("Vec", cas::ValueType);
	type->AddMember("int x", offsetof(Vec, x));
	type->AddMethod("Vec& operator += (int a)", &Vec::operator+=);
	RunTest(R"code(
		Vec global;
		global.x = 7;
		Vec get_global() { return global; }
		Vec a = get_global();
		a.x += 1;
		Assert_AreEqual(7, global.x);
		Assert_AreEqual(8, a.x);
		Vec set_to_1(Vec v) { v.x = 1; return v; }
		Vec b = set_to_1(a);
		Assert_AreEqual(8, a.x);
		Assert_AreEqual(1, b.x);
		Vec c = set_to_1(a) += 4;
		Assert_AreEqual(8, a.x);
		Assert_AreEqual(5, c.x);
	)code");
}

//=========================================================================================
struct A : RefCounter
{
	int x;
	static A* global;

	A() : x(4) {}
};

static A* createA()
{
	A* a = new A();
	return a;
}

static void checkA(A* a)
{
	Assert::AreEqual(1, a->GetRefs()); // 1 refernce in script
	A::global = a;
	a->AddRef(); // now two references
}

TEST_METHOD(RefCountedType)
{
	auto type = module->AddRefType<A>("A");
	type->AddCtor("A()");
	type->AddMember("int x", offsetof(A, x));
	module->AddFunction("void checkA(A& a)", checkA);
	RunTest("void f(){A a; a.x += 3; checkA(a);}();");
	Assert::AreEqual(1, A::global->GetRefs());
	Assert::AreEqual(7, A::global->x);
	A::global->Release();
}

//=========================================================================================
TEST_METHOD(ReturnValueToCode)
{
	// void
	RunTest("return;");
	retval.IsVoid();

	// bool
	RunTest("return true;");
	retval.IsBool(true);

	// char
	RunTest("return 'z';");
	retval.IsChar('z');

	// int
	RunTest("return 3;");
	retval.IsInt(3);

	// float
	RunTest("return 3.14;");
	retval.IsFloat(3.14f);
}

//=========================================================================================
TEST_METHOD(MultipleReturnValueToCode)
{
	// will upcast to common type - float
	RunTest("return 7; return 14.11; return false;");
	retval.IsFloat(7.f);
}

//=========================================================================================
static void pow(int& a)
{
	a = a*a;
}

TEST_METHOD(CodeFunctionTakesRef)
{
	module->AddFunction("void pow(int& a)", pow);
	RunTest("int a = 3; pow(a); return a;");
	retval.IsInt(9);
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
	module->AddFunction("int& getref(bool is_a)", getref);
	global_a = 1;
	global_b = 2;
	RunTest("getref(true) = 7; getref(false) *= 3;");
	Assert::AreEqual(7, global_a);
	Assert::AreEqual(6, global_b);
}

//=========================================================================================
static int f1() { return 1; }
static int f1(int i) { return i; }

TEST_METHOD(CodeRegisterOverloadedFunctions)
{
	module->AddFunction("int f1()", AsFunction(f1, int, ()));
	module->AddFunction("int f1(int i)", AsFunction(f1, int, (int)));
	RunTest("return f1() + f1(3) == 4;");
	retval.IsBool(true);
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
	IClass* type = module->AddType<AObj>("AObj");
	type->AddMethod("int GetX()", &AObj::GetX);
	type->AddMethod("void SetX(int a)", &AObj::SetX);
	RunTest("AObj a; a.SetX(7); return a.GetX();");
	retval.IsInt(7);
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
	IClass* type = module->AddType<BObj>("BObj");
	type->AddMethod("int f()", AsMethod(BObj, f, int, ()));
	type->AddMethod("int f(int a)", AsMethod(BObj, f, int, (int)));
	RunTest("BObj b; return b.f() + b.f(4);");
	retval.IsInt(9);
}

//=========================================================================================
class D
{
public:
	int x;

	inline void operator += (int a)
	{
		x += a;
	}
};

static void D_sub(D& d, int a)
{
	d.x -= a;
}

TEST_METHOD(OverloadClassOperator)
{
	IClass* type = module->AddType<D>("D");
	type->AddMember("int x", offsetof(D, x));
	type->AddMethod("void operator += (int a)", &D::operator+=);
	type->AddMethod("void operator -= (int a)", D_sub);
	RunTest("D d; d.x = 10; d -= 4; Assert_AreEqual(6,d.x); d += 11; Assert_AreEqual(17,d.x);");
}

//=========================================================================================
struct E
{
	int x;

	int f() { return x; }
	int f(int a) { return x*a; }
};

TEST_METHOD(Functor)
{
	IClass* type = module->AddType<E>("E");
	type->AddMember("int x", offsetof(E, x));
	type->AddMethod("int operator () ()", AsMethod(E, f, int, ()));
	type->AddMethod("int operator () (int a)", AsMethod(E, f, int, (int)));
	RunTest("E e; e.x = 4; Assert_AreEqual(4,e()); Assert_AreEqual(12,e(3));");
}

//=========================================================================================
struct F
{
	int x, y;

	F(int a)
	{
		x = y = a;
	}

	F(float a)
	{
		x = y = (int)a;
	}

	F(int x, int y) : x(x), y(y)
	{
	}

	int to_int()
	{
		return x + y;
	}

	float to_float()
	{
		return (float)(x - y);
	}
};

TEST_METHOD(CodeOverloadCast)
{
	auto type = module->AddType<F>("F");
	type->AddMember("int x", offsetof(F, x));
	type->AddMember("int y", offsetof(F, y));
	type->AddCtor<int>("implicit F(int a)");
	type->AddCtor<float>("F(float a)");
	type->AddCtor<int, int>("F(int x, int y)");
	type->AddMethod("implicit int operator cast()", &F::to_int);
	type->AddMethod("float operator cast()", &F::to_float);
	RunTest(R"code(
		// implicit cast
		F a = F(3,14);
		int b = a;
		Assert_AreEqual(17, b);

		// explicit cast
		float c = a as float;
		Assert_AreEqual(-11.0, c);

		// implicit ctor
		a = 7;
		Assert_AreEqual(7, a.x);

		// explicit cast
		a = F(3.14);
		Assert_AreEqual(3, a.x);
	)code");
}

//=========================================================================================
struct G
{
};

TEST_METHOD(CodeClassDefaultOperators)
{
	module->AddType<G>("G");
	RunTest(R"code(
		G a, b;
		Assert_IsTrue(a == a);
		Assert_IsFalse(a != a);
		Assert_IsFalse(a == b);
		Assert_IsTrue(a != b);
		a = b;
		Assert_IsTrue(a == b);
	)code");
}

//=========================================================================================
static void pass_string_by_val(string s)
{
	Assert::AreEqual("dodo", s.c_str());
	s = "123";
}

static string pass_string_by_val2(string s1, string s2, string s3)
{
	return s1 + s2 + s3;
}

TEST_METHOD(CodePassStringByValue)
{
	module->AddFunction("void f(string s)", pass_string_by_val);
	module->AddFunction("string f2(string s1, string s2, string s3)", pass_string_by_val2);
	RunTest(R"code(
		string s = "dodo";
		f(s);
		Assert_AreEqual("dodo", s);
		s = f2("a", "b", "c");
		Assert_AreEqual("abc", s);
	)code");
}

//=========================================================================================
struct I
{
	int x, y;
	I(int x, int y) : x(x), y(y) {}
	I(const I& i) : x(i.x), y(i.y) {}
};

static int pass_struct_by_val(I a, I b)
{
	++a.x;
	--a.y;
	b.x += 2;
	b.y -= 3;
	return a.x*b.x + a.y*b.y;
}

static void pass_struct_by_ref(I& a, I& b)
{
	a.x += b.x;
	a.y -= b.y;
	b.x *= 2;
	b.y /= 2;
}

TEST_METHOD(CodePassStructByValueAndReference)
{
	auto type = module->AddType<I>("I", cas::ValueType);
	type->AddCtor<int, int>("I(int x, int y)");
	type->AddCtor<const I&>("I(I& i)");
	type->AddMember("int x", offsetof(I, x));
	type->AddMember("int y", offsetof(I, y));
	module->AddFunction("int f(I a, I b)", pass_struct_by_val);
	module->AddFunction("void f2(I& a, I& b)", pass_struct_by_ref);
	RunTest(R"code(
		I a = I(1,7), b = I(2, 4), a0 = a, b0 = b;
		int r = f(a,b);
		Assert_AreEqual(14,r);
		Assert_IsTrue(a == a0);
		Assert_IsTrue(b == b0);
		f2(a,b);
		Assert_IsTrue(a == I(3,3));
		Assert_IsTrue(b == I(4,2));
	)code");
}

//=========================================================================================
static I get_struct_by_val(int x, int y)
{
	return I(x, y);
}
TEST_METHOD(ReturnCodeStructByValue)
{
	auto type = module->AddType<I>("I", cas::ValueType);
	type->AddMember("int x", offsetof(I, x));
	type->AddMember("int y", offsetof(I, y));
	module->AddFunction("I f(int x, int y)", get_struct_by_val);
	RunTest(R"code(
		I i = f(3,14);
		Assert_AreEqual(3,i.x);
		Assert_AreEqual(14,i.y);
	)code");
}

//=========================================================================================
static I global_i;
static I& get_struct_by_ref(int x, int y)
{
	global_i.x = x;
	global_i.y = y;
	return global_i;
}
TEST_METHOD(ReturnCodeStructByRef)
{
	auto type = module->AddType<I>("I", cas::ValueType);
	type->AddMember("int x", offsetof(I, x));
	type->AddMember("int y", offsetof(I, y));
	module->AddFunction("I& f(int x, int y)", get_struct_by_ref);
	RunTest(R"code(
		I& i = f(3,14);
		i.x += 2;
		i.y -= 3;
		Assert_AreEqual(5,i.x);
		Assert_AreEqual(11,i.y);
	)code");
	Assert::AreEqual(5, global_i.x);
	Assert::AreEqual(11, global_i.y);
}

//=========================================================================================
static void get_code_struct(I& i)
{
	Assert::AreEqual(7, i.x);
	Assert::AreEqual(9, i.y);
	--i.x;
	++i.y;
}
TEST_METHOD(PassCodeStructToCodeFunc)
{
	auto type = module->AddType<I>("I", cas::ValueType);
	type->AddMember("int x", offsetof(I, x));
	type->AddMember("int y", offsetof(I, y));
	module->AddFunction("I& f(int x, int y)", get_struct_by_ref);
	module->AddFunction("void f2(I& i)", get_code_struct);
	RunTest(R"code(
		I& i = f(7,9);
		f2(i);
		Assert_AreEqual(6,i.x);
		Assert_AreEqual(10,i.y);
	)code");
}

//=========================================================================================
static string global_str;
static string& return_string_by_ref()
{
	return global_str;
}
TEST_METHOD(CodeReturnStringByReference)
{
	module->AddFunction("string& f()", return_string_by_ref);
	global_str = "123";
	RunTest(R"code(
		string& s = f();
		Assert_AreEqual("123", s);
		s += "456";
		Assert_AreEqual("123456", s);
		string s2 = f();
		s2 += "789";
		Assert_AreEqual("123456", s);
	)code");
	Assert::AreEqual("123456", global_str.c_str());
}

//=========================================================================================
TEST_METHOD(PassCodeRefToScriptFunc)
{
	global_str.clear();
	module->AddFunction("string& f()", return_string_by_ref);
	RunTest(R"code(
		void f2(string& s) { s = "test"; }
		f2(f());
	)code");
	Assert::AreEqual("test", global_str.c_str());
}

//=========================================================================================
static void func_taking_str(string& s)
{
	s = "test";
}
TEST_METHOD(PassCodeRefToCodeFunc)
{
	global_str.clear();
	module->AddFunction("string& f()", return_string_by_ref);
	module->AddFunction("void f2(string& s)", func_taking_str);
	RunTest("f2(f());");
	Assert::AreEqual("test", global_str.c_str());
}

//=========================================================================================
static int& get_simple_ref()
{
	return global_a;
}
static void mod_simple_ref(int& a)
{
	a = 7;
}
TEST_METHOD(PassSimpleCodeRefToCodeFunc)
{
	global_a = 0;
	module->AddFunction("int& f()", get_simple_ref);
	module->AddFunction("void f2(int& a)", mod_simple_ref);
	RunTest("f2(f());");
	Assert::AreEqual(7, global_a);
}

//=========================================================================================
static string global_str2;
static string& return_string_by_ref2(int index)
{
	if(index == 0)
		return global_str;
	else
		return global_str2;
}
TEST_METHOD(CompareCodeRefs)
{
	module->AddFunction("string& f(int index)", return_string_by_ref2);
	RunTest(R"code(
		Assert_IsTrue(f(0) is f(0));
		Assert_IsTrue(f(1) is f(1));
		Assert_IsFalse(f(0) is f(1));
		Assert_IsFalse(f(1) is f(0));
	)code");
}

//=========================================================================================
class J
{
public:
	int x, y, z;
};
static J* create_class(int x, int y, int z)
{
	J* j = new J;
	j->x = x;
	j->y = y;
	j->z = z;
	return j;
}
static void func_taking_class(J& j)
{
	j.x += 1;
	j.y += 2;
	j.z += 3;
}
TEST_METHOD(PassCodeClassToCodeFunc)
{
	auto type = module->AddType<J>("J");
	type->AddMember("int x", offsetof(J, x));
	type->AddMember("int y", offsetof(J, y));
	type->AddMember("int z", offsetof(J, z));
	module->AddFunction("J& f(int x, int y, int z)", create_class);
	module->AddFunction("void f2(J& j)", func_taking_class);
	RunTest(R"code(
		J j = f(9,8,7);
		f2(j);
		Assert_AreEqual(10, j.x);
		Assert_AreEqual(10, j.y);
		Assert_AreEqual(10, j.z);
	)code");
}

//=========================================================================================
struct K
{
	int x;
	float y;
};
TEST_METHOD(CodeClassDefaultCtor)
{
	auto type = module->AddType<K>("K");
	type->AddMember("int x", offsetof(K, x));
	type->AddMember("float y", offsetof(K, y));
	RunTest(R"code(
		K k;
		Assert_AreEqual(0, k.x);
		Assert_AreEqual(0.0, k.y);
	)code");
}

//=========================================================================================
TEST_METHOD(StaticMethodInCode)
{
	RunTest(R"code(
		string s = "123";
		int a = int.Parse(s);
		Assert_AreEqual(123, a);
	)code");

	RunFailureTest(R"code(
		string s = "dupa";
		int a = int.Parse(s);
	)code", "Exception: Invalid int format 'dupa'.");
}

//=========================================================================================
TEST_METHOD(CodeEnum)
{
	enum E
	{
		E1,
		E2,
		E3
	};

	enum class F
	{
		F1,
		F2,
		F3
	};

	IEnum* enu = module->AddEnum("Enum");
	enu->AddValue("invalid");
	enu->AddValue("none", -1);
	enu->AddValues({ "a","b","c" });
	enu->AddValues({
		{"E1",E1},
		{"E2",E2},
		{"E3",E3}
	});
	enu->AddEnums<F>({
		{"F1",F::F1},
		{"F2",F::F2},
		{"F3",F::F3}
	});

	RunTest(R"code(
		Enum g = Enum.F2;
		void f(Enum e)
		{
			switch(e)
			{
			case Enum.E1:
				println("E1");
				break;
			case Enum.none:
				println("none");
				break;
			default:
				println(e);
				break;
			}
		}
		f(g);
		f(Enum.invalid);
	)code");
}

//=========================================================================================
CA_TEST_CLASS_END();

namespace tests
{
	int Code::global_a, Code::global_b;
	Code::A* Code::A::global;
	Code::I Code::global_i(0,0);
	string Code::global_str, Code::global_str2;
};
