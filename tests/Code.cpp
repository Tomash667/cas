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
	// Vec2
	module->AddType<Vec2>("Vec2");
	module->AddMember("Vec2", "float x", offsetof(Vec2, x));
	module->AddMember("Vec2", "float y", offsetof(Vec2, y));
	module->AddFunction("Vec2 create_vec2(float x, float y)", f_create_vec2);
	module->AddFunction("void wypisz_vec2(Vec2 v)", f_wypisz_vec2);
	// Vec3 (pod > 8 byte)
	module->AddType<Vec3>("Vec3");
	module->AddMember("Vec3", "float x", offsetof(Vec3, x));
	module->AddMember("Vec3", "float y", offsetof(Vec3, y));
	module->AddMember("Vec3", "float z", offsetof(Vec3, z));
	module->AddFunction("Vec3 create_vec3(float x, float y, float z)", f_create_vec3);
	module->AddFunction("void wypisz_vec3(Vec3 v)", f_wypisz_vec3);
	// INT2c have ctor
	module->AddType<INT2>("INT2c");
	module->AddMember("INT2c", "int x", offsetof(INT2, x));
	module->AddMember("INT2c", "int y", offsetof(INT2, y));
	module->AddMethod("INT2c", "INT2c()", f_int2c_ctor0);
	module->AddMethod("INT2c", "INT2c(int xy)", f_int2c_ctor1);
	module->AddMethod("INT2c", "INT2c(int x, int y)", f_int2c_ctor2);
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

TEST_METHOD(CodeRegisterValueType)
{
	module->AddType<Vec>("Vec", cas::ValueType);
	module->AddMember("Vec", "int x", offsetof(Vec, x));
	module->AddMethod("Vec", "Vec operator += (int a)", &Vec::operator+=);
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

	A() : x(4) {}
};

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
	module->AddType<AObj>("AObj");
	module->AddMethod("AObj", "int GetX()", &AObj::GetX);
	module->AddMethod("AObj", "void SetX(int a)", &AObj::SetX);
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
	module->AddType<CObj>("BObj");
	module->AddMethod("BObj", "int f()", AsMethod(BObj, f, int, ()));
	module->AddMethod("BObj", "int f(int a)", AsMethod(BObj, f, int, (int)));
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
	module->AddType<D>("D");
	module->AddMember("D", "int x", offsetof(D, x));
	module->AddMethod("D", "void operator += (int a)", &D::operator+=);
	module->AddMethod("D", "void operator -= (int a)", D_sub);
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
	module->AddType<E>("E");
	module->AddMember("E", "int x", offsetof(E, x));
	module->AddMethod("E", "int operator () ()", AsMethod(E, f, int, ()));
	module->AddMethod("E", "int operator () (int a)", AsMethod(E, f, int, (int)));
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
	module->AddType<F>("F");
	module->AddMember("F", "int x", offsetof(F, x));
	module->AddMember("F", "int y", offsetof(F, y));
	module->AddMethod("F", "implicit F(int a)", AsCtor<F, int>());
	module->AddMethod("F", "F(float a)", AsCtor<F, float>());
	module->AddMethod("F", "F(int x, int y)", AsCtor<F, int, int>());
	module->AddMethod("F", "implicit int operator cast()", &F::to_int);
	module->AddMethod("F", "float operator cast()", &F::to_float);
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
	module->AddType<Pod>("Pod");
	module->AddMember("Pod", "int x", offsetof(Pod, x));
	module->AddMember("Pod", "int y", offsetof(Pod, y));

	module->AddType<Class>("Class");
	module->AddMember("Class", "int x", offsetof(Class, x));
	module->AddMember("Class", "int y", offsetof(Class, y));
	module->AddMethod("Class", "Class()", AsCtor<Class>());
	module->AddMethod("Class", "Class(Class& c)", AsCtor<Class, Class&>());
	module->AddMethod("Class", "Class(int x, int y)", AsCtor<Class, int, int>());
}

static int get4() { return 4; }
static Pod getPodValue() { Pod p; p.x = 1; p.y = 2; return p; }

TEST_METHOD(CodeReturnByValue)
{
	RegisterPodAndClass();
	RunTest(R"code(


	)code");
}

TEST_METHOD(CodeReturnByReference)
{

}

TEST_METHOD(CodeReturnByPointer)
{

}

TEST_METHOD(CodeTakesByValue)
{

}

TEST_METHOD(CodeTakesByReference)
{

}

TEST_METHOD(CodeTakesByPointer)
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

TEST_METHOD(CodeReturnByValueReferencePointer)
{
	globalh.x = 3;
	globalh.y = 14;
	module->AddType<H>("H");
	module->AddMember("H", "int x", offsetof(H, x));
	module->AddMember("H", "int y", offsetof(H, y));
	RunTest(R"code(
		
	)code");
}

CA_TEST_CLASS_END();

int tests::Code::global_a;
int tests::Code::global_b;
