#include "Pch.h"
#include "TestBase.h"

class Cases : public TestBase
{
public:
	void RunFileTest(cstring filename, cstring input, cstring exp_output, bool optimize = true)
	{
		if(!env.CI_MODE && input[0] != 0)
			TestEnvironment::Info(Format("Script input: [%s]", input));

		string path(Format("../cases/%s", filename));
		std::ifstream ifs(path);
		ASSERT_TRUE(ifs.is_open()) << Format(Format("Failed to open file '%s'.", path.c_str()));
		std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
		ifs.close();

		env.ResetIO(input);

		Result result = ParseAndRunWithTimeout(module, content.c_str(), optimize);
		string output = env.s_output.str();
		if(!env.CI_MODE)
			env.Info(Format("Script output: [%s]", output.c_str()));

		switch(result)
		{
		case Result::OK:
			EXPECT_STREQ(output.c_str(), exp_output) << "Invalid output.";
			break;
		case Result::TIMEOUT:
			ADD_FAILURE() << Format("Script execution/parsing timeout. Parse output: [%s]", env.event_output.c_str());
			break;
		case Result::ASSERT:
			ADD_FAILURE() << Format("Code assert: [%s]", env.event_output.c_str());
			break;
		case Result::FAILED:
			ADD_FAILURE() << Format("Script parsing failed. Parse output: [%s]", env.event_output.c_str());
			break;
		}
	}
};

TEST_F(Cases, Simple)
{
	RunFileTest("simple.txt", "4", "Podaj a: Odwrocone a: -4");
}

TEST_F(Cases, Math)
{
	RunFileTest("math.txt", "1 2 3 4", "7\n2\n3\n-2\n0\n37\n1\n");
	RunFileTest("math.txt", "8 15 4 3", "68\n119\n23\n-120\n0\n37\n0\n");
}

TEST_F(Cases, Assign)
{
	RunFileTest("assign.txt", "", "5\n1\n8\n4\n2\n7\ntrue\n");
}

TEST_F(Cases, String)
{
	RunFileTest("string.txt", "Tomash 1990", "Podaj imie: Podaj rok urodzenia: Witaj Tomash! Masz 26 lat.");
}

TEST_F(Cases, Float)
{
	RunFileTest("float.txt", "7", "153.934\n5.5\n5.5\n3\n");
}

TEST_F(Cases, Parentheses)
{
	RunFileTest("parentheses.txt", "", "-5\n20\n0\n24\n");
}

TEST_F(Cases, Bool)
{
	RunFileTest("bool.txt", "7 8", "false\ntrue\nfalse\nfalse\ntrue\ntrue\n-1\n8\ntrue\nfalse\n");
}

TEST_F(Cases, CompOperators)
{
	RunFileTest("comp_operators.txt", "", "true\ntrue\ntrue\ntrue\ntrue\nfalse\nfalse\ntrue\nfalse\n");
}

TEST_F(Cases, IfElse)
{
	RunFileTest("if_else.txt", "1 1", "a == b\n0\n1\n4\n6\n7\n");
	RunFileTest("if_else.txt", "2 1", "a > b\n2\n4\n6\n7\n");
	RunFileTest("if_else.txt", "1 2", "a < b\n2\n4\n6\n7\n", false);
}

TEST_F(Cases, While)
{
	RunFileTest("while.txt", "3", "***yyyy/++--");
	RunFileTest("while.txt", "4", "****yyyy/++--", false);
}

TEST_F(Cases, TypeFunc)
{
	RunFileTest("type_func.txt", "-9", "4\n9\n6\n3.1415\n");
}

TEST_F(Cases, Block)
{
	RunFileTest("block.txt", "", "4\n0\n8\n");
}

TEST_F(Cases, For)
{
	RunFileTest("for.txt", "", "0123456789***+++++-----");
	RunFileTest("for.txt", "", "0123456789***+++++-----", false);
}

TEST_F(Cases, IncDec)
{
	RunFileTest("inc_dec.txt", "7", "7\n8\n7\n7\n8\n7\n");
}

TEST_F(Cases, UserFunc)
{
	RunFileTest("user_func.txt", "1 2 3 4 5 6 7 8", "3\n6\n9\n15\n15\n");
}

TEST_F(Cases, FuncDefParams)
{
	RunFileTest("func_def_params.txt", "", "a: 3, b: 4\na: 7, b: 4\na: 11, b: 13\n");
}

TEST_F(Cases, DefValue)
{
	RunFileTest("def_value.txt", "", "false\n0\n0\n[]\nfalse\n0\n0\n[]\n");
}

TEST_F(Cases, BitOp)
{
	RunFileTest("bit_op.txt", "", "17029\n65534\n38208\n792166400\n7776\n33280\n41701\n28212\n-1194328064\n-9330688\n");
}

TEST_F(Cases, Class)
{
	RunFileTest("class.txt", "", "x:3 y:4\nx:7 y:11\nx:5 y:8\n7\n6\n55\nx:5 y:4\n");
}

TEST_F(Cases, ComplexClassResult)
{
	RunFileTest("complex_class_result.txt", "", "x:1.6 y:2.3 z:4.1\nx:1.6 y:2.4 z:4.1\nx:0 y:0\nx:13 y:13\nx:7 y:3\nx:3.14 y:0.0015\n");
}

TEST_F(Cases, FuncOverload)
{
	RunFileTest("func_overload.txt", "", "4\n\nvoid\nint 1\nstring test\nfloat 3.14, int 3\n");
}

TEST_F(Cases, ScriptClass)
{
	RunFileTest("script_class.txt", "", "x:3\ny:0.14\nc:true\nd:false\ng:10\nh:false\n");
}

TEST_F(Cases, ScriptClassFunc)
{
	RunFileTest("script_class_func.txt", "", "3.5\n3.5\n19.5\n20.5\n");
}

TEST_F(Cases, Is)
{
	RunFileTest("is.txt", "", "1\n2\n3\n4\n5\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\n");
}

TEST_F(Cases, Ref)
{
	RunFileTest("ref.txt", "11 13", "12\n12\n26,4\n4,999\n999,4\n");
}

TEST_F(Cases, ClassRef)
{
	RunFileTest("class_ref.txt", "", "20\n4\n11,22\n12\n");
}

TEST_F(Cases, Cast)
{
	RunFileTest("cast.txt", "", "3\n4.4\n5.7\n4\n5\n3\n1\n4\nok\n");
}
