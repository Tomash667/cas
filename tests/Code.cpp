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

static INT2 f_int2c_ctor0()
{
	return INT2(0, 0);
}

static INT2 f_int2c_ctor1(int xy)
{
	return INT2(xy);
}

static INT2 f_int2c_ctor2(int x, int y)
{
	return INT2(x, y);
}

static void f_wypisz_int2c(INT2& i)
{
	s_output << "x:" << i.x << " y:" << i.y << "\n";
}

struct Vec2
{
	float x, y;
};

static Vec2 f_create_vec2(float x, float y)
{
	Vec2 v;
	v.x = x;
	v.y = y;
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

static Vec3 f_create_vec3(float x, float y, float z)
{
	Vec3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

static void f_wypisz_vec3(Vec3& v)
{
	s_output << "x:" << v.x << " y:" << v.y << " z:" << v.z << "\n";
}

TEST_METHOD(ComplexClassResult)
{
	IType* type;
	// Vec2
	type = module->AddType<Vec2>("Vec2");
	type->AddMember("float x", offsetof(Vec2, x));
	type->AddMember("float y", offsetof(Vec2, y));
	module->AddFunction("Vec2 create_vec2(float x, float y)", f_create_vec2);
	module->AddFunction("void wypisz_vec2(Vec2 v)", f_wypisz_vec2);
	// Vec3 (pod > 8 byte)
	type = module->AddType<Vec3>("Vec3");
	type->AddMember("float x", offsetof(Vec3, x));
	type->AddMember("float y", offsetof(Vec3, y));
	type->AddMember("float z", offsetof(Vec3, z));
	module->AddFunction("Vec3 create_vec3(float x, float y, float z)", f_create_vec3);
	module->AddFunction("void wypisz_vec3(Vec3 v)", f_wypisz_vec3);
	// INT2c have ctor
	type = module->AddType<INT2>("INT2c");
	type->AddMember("int x", offsetof(INT2, x));
	type->AddMember("int y", offsetof(INT2, y));
	type->AddMethod("INT2c()", f_int2c_ctor0);
	type->AddMethod("INT2c(int xy)", f_int2c_ctor1);
	type->AddMethod("INT2c(int x, int y)", f_int2c_ctor2);
	module->AddFunction("void wypisz_int2c(INT2c i)", f_wypisz_int2c);

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

TEST_METHOD_IGNORE(CodeRegisterValueType)
{
	IType* type = module->AddType<Vec>("Vec", cas::ValueType);
	type->AddMember("int x", offsetof(Vec, x));
	type->AddMethod("Vec operator += (int a)", &Vec::operator+=);
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
	Assert::AreEqual(2, a->GetRefs());
	A::global = a;
	a->AddRef();
}

TEST_METHOD_IGNORE(RefCountedType)
{
	IType* type = module->AddRefType<A>("A");
	type->AddMethod("A()", AsCtor<A>());
	type->AddMember("int x", offsetof(A, x));
	module->AddFunction("void checkA(A@ a)", checkA);
	RunTest("void f(){A a; a.x += 3; checkA(a);}();");
	Assert::AreEqual(1, A::global->GetRefs());
	Assert::AreEqual(7, A::global->x);
}

/*TEST_METHOD(CodeRegisterType)
{
	ModuleRef module;
	module->AddRefType<A>("A");
	module->AddMethod("A", "A()", AsCtor<A>());
	module.RunTest("A a; return a.x += 2;");
	module.ret().IsInt(6);
}*/

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

TEST_METHOD_IGNORE(CodeFunctionTakesRef)
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

TEST_METHOD_IGNORE(CodeFunctionReturnsRef)
{
	module->AddFunction("int& getref(bool is_a)", getref);
	global_a = 1;
	global_b = 2;
	RunTest("getref(true) = 7; getref(false) *= 3;");
	Assert::AreEqual(7, global_a);
	Assert::AreEqual(6, global_b);
}

//=========================================================================================
TEST_METHOD_IGNORE(IsCompareCodeRefs)
{
	module->AddFunction("int& getref(bool is_a)", getref);

	RunTest("return getref(true) is getref(true);");
	retval.IsBool(true);

	RunTest("return getref(true) is getref(false);");
	retval.IsBool(false);
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
	IType* type = module->AddType<AObj>("AObj");
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
	IType* type = module->AddType<CObj>("BObj");
	type->AddMethod("int f()", AsMethod(BObj, f, int, ()));
	type->AddMethod("int f(int a)", AsMethod(BObj, f, int, (int)));
	RunTest("BObj b; return b.f() + b.f(4);");
	retval.IsInt(9);
}

//=========================================================================================


class CObj : public RefCounter
{
public:
	int x;

	inline CObj() : x(1) {}
	inline CObj(int x) : x(x) {}
	inline int GetX() { return x; }
};

/*TEST_METHOD(CodeCtorMemberFunction)
{
	ModuleRef module;
	module->AddType<CObj>("CObj");
	module->AddMethod("CObj", "CObj()", AsCtor<CObj>());
	module->AddMethod("CObj", "CObj(int a)", AsCtor<CObj, int>());
	module->AddMethod("CObj", "int GetX()", &CObj::GetX);
	module.RunTest("CObj c1; CObj c7 = CObj(7); return c1.GetX() + c7.GetX();");
	module.ret().IsInt(8);
}*/


// CodeCtorMemberFunctionOverload

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
	IType* type = module->AddType<D>("D");
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
	IType* type = module->AddType<E>("E");
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
	IType* type = module->AddType<F>("F");
	type->AddMember("int x", offsetof(F, x));
	type->AddMember("int y", offsetof(F, y));
	type->AddMethod("implicit F(int a)", AsCtor<F, int>());
	type->AddMethod("F(float a)", AsCtor<F, float>());
	type->AddMethod("F(int x, int y)", AsCtor<F, int, int>());
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
struct Pod
{
	int x;
	int y;
};

class Class
{
public:
	Class() { x = 0; y = 0; }
	Class(Class& c) { x = c.x; y = c.y; }
	Class(int x, int y) : x(x), y(y) {}
	void operator = (Class& c) { x = c.x; y = c.y; }
	int x;
	int y;
};

void RegisterPodAndClass()
{
	IType* type = module->AddType<Pod>("Pod");
	type->AddMember("int x", offsetof(Pod, x));
	type->AddMember("int y", offsetof(Pod, y));

	type = module->AddType<Class>("Class");
	type->AddMember("int x", offsetof(Class, x));
	type->AddMember("int y", offsetof(Class, y));
	type->AddMethod("Class()", AsCtor<Class>());
	type->AddMethod("Class(Class& c)", AsCtor<Class, Class&>());
	type->AddMethod("Class(int x, int y)", AsCtor<Class, int, int>());
}

static int get4() { return 4; }
static Pod getPodValue() { Pod p; p.x = 1; p.y = 2; return p; }

TEST_METHOD_IGNORE(CodeReturnByValue)
{
	RegisterPodAndClass();
	RunTest(R"code(


	)code");
}

TEST_METHOD_IGNORE(CodeReturnByReference)
{

}

TEST_METHOD_IGNORE(CodeReturnByPointer)
{

}

TEST_METHOD_IGNORE(CodeTakesByValue)
{

}

TEST_METHOD_IGNORE(CodeTakesByReference)
{

}

TEST_METHOD_IGNORE(CodeTakesByPointer)
{

}

struct H
{
	int x, y;
};

static H globalh;

H createH(int x, int y)
{
	H h;
	h.x = x;
	h.y = y;
	return h;
}

H& getGlobalH()
{
	return globalh;
}

H* getGlobalHPtr()
{
	return &globalh;
}

TEST_METHOD_IGNORE(CodeReturnByValueReferencePointer)
{
	globalh.x = 3;
	globalh.y = 14;
	IType* type = module->AddType<H>("H");
	type->AddMember("int x", offsetof(H, x));
	type->AddMember("int y", offsetof(H, y));
	RunTest(R"code(
		
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
	IType* type = module->AddType<I>("I", cas::ValueType);
	type->AddMethod("I(int x, int y)", AsCtor<I, int, int>());
	type->AddMethod("I(I& i)", AsCtor<I, const I&>());
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
CA_TEST_CLASS_END();

int tests::Code::global_a;
int tests::Code::global_b;
tests::Code::A* tests::Code::A::global;
tests::Code::H tests::Code::globalh;
