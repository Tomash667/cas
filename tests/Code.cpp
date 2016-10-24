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
	float x, y, z;
};

static Vec Get1()
{
	Vec v;
	v.x = 1;
	v.y = 1;
	v.z = 1;
	return v;
}

static void Set123(Vec& v)
{
	v.x = 1;
	v.y = 2;
	v.z = 3;
}

static float Sum(const Vec& v)
{
	return v.x + v.y + v.z;
}

static float Sum2(Vec v)
{
	v.x *= 2;
	v.y *= 2;
	v.z *= 2;
	return Sum(v);
}

/*TEST_METHOD(CodeRegisterValueType)
{
	ModuleRef module;
	module->AddType<Vec>("Vec");
	module->AddFunction("Vec Get1()", Get1);
	module->AddFunction("void Set123(Vec& v)", Set123);
	module->AddFunction("float Sum(const Vec& v)", Sum);
	module->AddFunction("float Sum2()", Sum2);
	module.RunTest("Vec v = Get1(); v.x = 0; v.y += 1; float sum = Sum(v); Set123(v); sum += Sum2(v); sum += v; return sum;");
	module.ret().IsInt(21);
}*/

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
TEST_METHOD(IsCompareCodeRefs)
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

struct AsCtorHelper
{
	template<typename T, typename... Args>
	static T* Create(Args&&... args)
	{
		return new T(args...);
	}
};

template<typename T, typename... Args>
inline FunctionInfo AsCtor()
{
	return FunctionInfo(AsCtorHelper::Create<T, Args...>);
}

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

CA_TEST_CLASS_END();

int tests::Code::global_a;
int tests::Code::global_b;
