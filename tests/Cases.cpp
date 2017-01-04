#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define TestMethod(name) \
	TEST_METHOD(name) \
	{ \
		if(!CI_MODE) \
			Logger::WriteMessage("\n***** Test case: " #name " ******************************\n"); \
		name##_impl(); \
	} \
	void name##_impl()

CA_TEST_CLASS(Cases);

TestMethod(Simple)
{
	RunFileTest("simple.txt", "4", "Podaj a: Odwrocone a: -4");
}

TestMethod(Math)
{
	RunFileTest("math.txt", "1 2 3 4", "7\n2\n3\n-2\n0\n37\n1\n");
	RunFileTest("math.txt", "8 15 4 3", "68\n119\n23\n-120\n0\n37\n0\n");
}

TestMethod(Assign)
{
	RunFileTest("assign.txt", "", "5\n1\n8\n4\n2\n7\ntrue\n");
}

TestMethod(Assign2)
{
	RunFileTest("assign2.txt");
}

TestMethod(String)
{
	RunFileTest("string.txt", "Tomash 1990", "Podaj imie: Podaj rok urodzenia: Witaj Tomash! Masz 26 lat.");
}

TestMethod(String2)
{
	RunFileTest("string2.txt");
}

TestMethod(Float)
{
	RunFileTest("float.txt", "7", "153.934\n5.5\n5.5\n3\n");
}

TestMethod(Parentheses)
{
	RunFileTest("parentheses.txt", "", "-5\n20\n0\n24\n");
}

TestMethod(Bool)
{
	RunFileTest("bool.txt", "7 8", "false\ntrue\nfalse\nfalse\ntrue\ntrue\n-1\n8\ntrue\nfalse\n");
}

TestMethod(CompOperators)
{
	RunFileTest("comp_operators.txt");
}

TestMethod(IfElse)
{
	RunFileTest("if_else.txt", "1 1", "a == b\n0\n1\n4\n6\n7\n");
	RunFileTest("if_else.txt", "2 1", "a > b\n2\n4\n6\n7\n");
	RunFileTest("if_else.txt", "1 2", "a < b\n2\n4\n6\n7\n", false);
}

TestMethod(While)
{
	RunFileTest("while.txt", "3", "***yyyy/++--");
	RunFileTest("while.txt", "4", "****yyyy/++--", false);
}

TestMethod(TypeFunc)
{
	RunFileTest("type_func.txt", "-9", "4\n9\n6\n3.1415\n");
}

TestMethod(Block)
{
	RunFileTest("block.txt", "", "4\n0\n8\n");
}

TestMethod(For)
{
	RunFileTest("for.txt", "", "0123456789***+++++-----");
	RunFileTest("for.txt", "", "0123456789***+++++-----", false);
}

TestMethod(IncDec)
{
	RunFileTest("inc_dec.txt", "7", "7\n8\n7\n7\n8\n7\n");
}

TestMethod(UserFunc)
{
	RunFileTest("user_func.txt", "1 2 3 4 5 6 7 8", "3\n6\n9\n15\n15\n");
}

TestMethod(FuncDefParams)
{
	RunFileTest("func_def_params.txt", "", "a: 3, b: 4\na: 7, b: 4\na: 11, b: 13\n");
}

TestMethod(DefValue)
{
	RunFileTest("def_value.txt", "", "false\n0\n0\n[]\nfalse\n0\n0\n[]\n");
}

TestMethod(BitOp)
{
	RunFileTest("bit_op.txt", "", "17029\n65534\n38208\n792166400\n7776\n33280\n41701\n28212\n-1194328064\n-9330688\n9330687\n");
}

TestMethod(Class)
{
	RunFileTest("class.txt", "", "x:3 y:4\nx:7 y:11\nx:5 y:8\n7\n6\n55\nx:5 y:4\n");
}

TestMethod(FuncOverload)
{
	RunFileTest("func_overload.txt", "", "4\n\nvoid\nint 1\nstring test\nfloat 3.14, int 3\n");
}

TestMethod(ScriptClass)
{
	RunFileTest("script_class.txt", "", "x:3\ny:0.14\nc:true\nd:false\ng:10\nh:false\n");
}

TestMethod(ScriptClassFunc)
{
	RunFileTest("script_class_func.txt", "", "3.5\n3.5\n19.5\n20.5\n");
}

TestMethod(Is)
{
	RunFileTest("is.txt", "", "1\n2\n3\n4\n5\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\n");
}

TestMethod(Ref)
{
	RunFileTest("ref.txt", "11 13", "12\n12\n26,4\n4,999\n999,4\n");
}

TestMethod(Ref2)
{
	RunFileTest("ref2.txt");
}

TestMethod(ClassRef)
{
	RunFileTest("class_ref.txt", "", "20\n4\n11,22\n12\n");
}

TestMethod(Cast)
{
	RunFileTest("cast.txt", "", "3\n4.4\n5.7\n4\n5\n3\n1\n4\nok\n");
}

TestMethod(Struct)
{
	RunFileTest("struct.txt");
}

TestMethod(Char)
{
	RunFileTest("char.txt", "a\nb\n", "test!");
	RunFileTest("char.txt", "a\na\n", "testa");
	RunFileTest("char.txt", "?\nb\n", "_test!");
}

TestMethod(Subscript)
{
	RunFileTest("subscript.txt");
}

TestMethod(Ternary)
{
	RunFileTest("ternary.txt");
	RunFileTest("ternary.txt", "", "", false);
}

TestMethod(Switch)
{
	RunFileTest("switch.txt");
}

TestMethod(OverloadCast)
{
	RunFileTest("overload_cast.txt");
}

TestMethod(Forward)
{
	RunFileTest("forward.txt", "", "f\nf23\n10\nY::f30\nY::f13\nZ::f\nZ::f\n");
}

TestMethod(LongRefAssign)
{
	RunFileTest("long_ref_assign.txt");
}

TestMethod(VardeclAssign)
{
	RunFileTest("vardecl_assign.txt", "", "ctor0\n---\nctor2\n---\nctor2\nassign\n---\nctor2\n---\nctor2\nassign\n---\nctor2\nctor2\n---\nassign\nctor2\n"
		"ctor2\n");
}

TestMethod(Enum)
{
	RunFileTest("enum.txt");
}

CA_TEST_CLASS_END();
