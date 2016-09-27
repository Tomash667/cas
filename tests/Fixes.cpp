#include "Pch.h"
#include "TestBase.h"

//class Fixes : public TestBase
//{
//public:
//};
//
//TEST_F(Fixes, CallingEmptyFunctionInfiniteLoop)
//{
//	/*
//	this caused something like this
//	f:
//	; missing ret
//	main:
//	call f
//	*/
//	RunTest("void f() {} f();");
//}
//
//TEST_F(Fixes, ReturnInVoidFunction)
//{
//	RunTest("void f() {return;}");
//}