#include "stdafx.h"
#include "CppUnitTest.h"

#define DO_EXPAND(VAL) VAL ## 0
#define EXPAND(VAL) DO_EXPAND(VAL)
#if defined(_CI_MODE) && (EXPAND(_CI_MODE) == 10)
#	define CI_MODE
#endif

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

enum Result
{
	OK,
	TIMEOUT,
	FAILED
};

struct PackedData
{
	cstring input;
	bool optimize;
};

#ifdef CI_MODE
const int DEFAULT_TIMEOUT = 60;
#else
const int DEFAULT_TIMEOUT = 1;
#endif

unsigned __stdcall ThreadStart(void* data)
{
	PackedData* pdata = (PackedData*)data;
	bool result = cas::ParseAndRun(pdata->input, pdata->optimize);
	return (result ? 1u : 0u);
}

string event_output;

void TestEventHandler(cas::EventType event_type, cstring msg)
{
	cstring m = Format("%s: %s\n", event_type == cas::Error ? "ERROR" : "WARN", msg);
	Logger::WriteMessage(m);
	event_output += m;
}

std::istringstream s_input;
std::ostringstream s_output;

namespace tests
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			cas::SetHandler(TestEventHandler);
			cas::Settings s;
			s.input = &s_input;
			s.output = &s_output;
			s.use_getch = false;
			cas::Initialize(&s);
			Assert::IsTrue(event_output.empty(), L"Cas initialization failed.");
		}

		wstring GetWC(cstring s)
		{
			const size_t len = strlen(s);
			wstring str;
			str.resize(len);
			mbstowcs((wchar_t*)str.data(), s, len);
			return str;
		}

		Result ParseAndRunWithTimeout(cstring content, bool optimize, int timeout = DEFAULT_TIMEOUT)
		{
			PackedData pdata;
			pdata.input = content;
			pdata.optimize = optimize;
			HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0u, ThreadStart, &pdata, 0u, nullptr);
			DWORD result = WaitForSingleObject(thread, timeout * 1000);
			Assert::IsTrue(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT, L"Failed to create parsing thread.");
			if(result == WAIT_TIMEOUT)
			{
				TerminateThread(thread, 2);
				return TIMEOUT;
			}
			DWORD exit_code;
			GetExitCodeThread(thread, &exit_code);
			return exit_code == 1u ? OK : FAILED;
		}

		void Test(cstring filename, cstring input, cstring output, bool optimize = true)
		{
			event_output.clear();

#ifndef CI_MODE
			if(input[0] != 0)
			{
				Logger::WriteMessage("Script input:\n");
				Logger::WriteMessage(input);
				Logger::WriteMessage("\n\n");
			}
#endif

			string path(Format("../../../cases/%s", filename));
			std::ifstream ifs(path);
			Assert::IsTrue(ifs.is_open(), GetWC(Format("Failed to open file '%s'.", path.c_str())).c_str());
			std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			s_input.clear();
			s_input.str(input);
			s_output.clear();
			s_output.str("");

			Result result = ParseAndRunWithTimeout(content.c_str(), optimize);
			string s = s_output.str();
			cstring ss = s.c_str();
			if(result == TIMEOUT)
			{
				cstring output = Format("Script execution/parsing timeout. Parse output:\n%s\nOutput: %s", event_output.c_str(), ss);
				Assert::Fail(GetWC(output).c_str());
			}
			else if(result == FAILED)
			{
				cstring output = Format("Script parsing failed. Parse output:\n%s\nOutput: %s", event_output.c_str(), ss);
				Assert::Fail(GetWC(output).c_str());
			}
#ifndef CI_MODE
			Logger::WriteMessage("Script output:\n");
			Logger::WriteMessage(ss);
#endif
			Assert::AreEqual(output, ss, "Invalid output.");
		}
		
		TEST_METHOD(Simple)
		{
			Logger::WriteMessage("Test case: Simple ******************************\n");
			Test("simple.txt", "4", "Podaj a: Odwrocone a: -4");
		}

		TEST_METHOD(Math)
		{
			Logger::WriteMessage("Test case: Math ******************************\n");
			Test("math.txt", "1 2 3 4", "7\n2\n3\n-2\n0\n37\n1\n");
			Test("math.txt", "8 15 4 3", "68\n119\n23\n-120\n0\n37\n0\n");
		}

		TEST_METHOD(Assign)
		{
			Logger::WriteMessage("Test case: Assign ******************************\n");
			Test("assign.txt", "", "5\n1\n8\n4\n2\n7\ntrue\n");
		}

		TEST_METHOD(String)
		{
			Logger::WriteMessage("Test case: String ******************************\n");
			Test("string.txt", "Tomash 1990", "Podaj imie: Podaj rok urodzenia: Witaj Tomash! Masz 26 lat.");
		}

		TEST_METHOD(Float)
		{
			Logger::WriteMessage("Test case: Float ******************************\n");
			Test("float.txt", "7", "153.934\n5.5\n5.5\n3\n");
		}

		TEST_METHOD(Parentheses)
		{
			Logger::WriteMessage("Test case: Parentheses ******************************\n");
			Test("parentheses.txt", "", "-5\n20\n0\n24\n");
		}

		TEST_METHOD(Bool)
		{
			Logger::WriteMessage("Test case: Bool ******************************\n");
			Test("bool.txt", "7 8", "false\ntrue\nfalse\nfalse\ntrue\ntrue\n-1\n8\ntrue\nfalse\n");
		}

		TEST_METHOD(CompOperators)
		{
			Logger::WriteMessage("Test case: CompOperators ******************************\n");
			Test("comp_operators.txt", "", "true\ntrue\ntrue\ntrue\ntrue\nfalse\nfalse\ntrue\nfalse\n");
		}

		TEST_METHOD(IfElse)
		{
			Logger::WriteMessage("Test case: IfElse ******************************\n");
			Test("if_else.txt", "1 1", "a == b\n0\n1\n4\n6\n7\n");
			Test("if_else.txt", "2 1", "a > b\n2\n4\n6\n7\n");
			Test("if_else.txt", "1 2", "a < b\n2\n4\n6\n7\n", false);
		}

		TEST_METHOD(While)
		{
			Logger::WriteMessage("Test case: While ******************************\n");
			Test("while.txt", "3", "***yyyy/++--");
			Test("while.txt", "4", "****yyyy/++--", false);
		}

		TEST_METHOD(TypeFunc)
		{
			Logger::WriteMessage("Test case: TypeFunc ******************************\n");
			Test("type_func.txt", "-9", "4\n9\n6\n3.1415\n");
		}

		TEST_METHOD(Block)
		{
			Logger::WriteMessage("Test case: Block ******************************\n");
			Test("block.txt", "", "4\n0\n8\n");
		}

		TEST_METHOD(For)
		{
			Logger::WriteMessage("Test case: For ******************************\n");
			Test("for.txt", "", "0123456789***+++++-----");
			Test("for.txt", "", "0123456789***+++++-----", false);
		}

		TEST_METHOD(IncDec)
		{
			Logger::WriteMessage("Test case: IncDec ******************************\n");
			Test("inc_dec.txt", "7", "8\n10\n10\n11\n");
		}

		TEST_METHOD(UserFunc)
		{
			Logger::WriteMessage("Test case: UserFunc ******************************\n");
			Test("user_func.txt", "1 2 3 4 5 6 7 8", "3\n6\n9\n15\n15\n");
		}

		TEST_METHOD(FuncDefParams)
		{
			Logger::WriteMessage("Test case: FuncDefParams ******************************\n");
			Test("func_def_params.txt", "", "a: 3, b: 4\na: 7, b: 4\na: 11, b: 13\n");
		}

		TEST_METHOD(DefValue)
		{
			Logger::WriteMessage("Test case: DefValue ******************************\n");
			Test("def_value.txt", "", "false\n0\n0\n[]\nfalse\n0\n0\n[]\n");
		}

		TEST_METHOD(BitOp)
		{
			Logger::WriteMessage("Test case: BitOp ******************************\n");
			Test("bit_op.txt", "", "17029\n65534\n38208\n792166400\n7776\n33280\n41701\n28212\n-1194328064\n-9330688\n");
		}

		TEST_METHOD(Class)
		{
			Logger::WriteMessage("Test case: Class ******************************\n");
			Test("class.txt", "", "x:3 y:4\nx:7 y:11\nx:5 y:8\n7\n6\n55\nx:5 y:4\n");
		}

		TEST_METHOD(ComplexClassResult)
		{
			Logger::WriteMessage("Test case: ComplexClassResult ******************************\n");
			Test("complex_class_result.txt", "", "x:1.6 y:2.3 z:4.1\nx:1.6 y:2.4 z:4.1\nx:0 y:0\nx:13 y:13\nx:7 y:3\nx:3.14 y:0.0015\n");
		}

		TEST_METHOD(FuncOverload)
		{
			Logger::WriteMessage("Test case: FuncOverload ******************************\n");
			Test("func_overload.txt", "", "4\n\nvoid\nint 1\nstring test\nfloat 3.14, int 3\n");
		}

		TEST_METHOD(ScriptClass)
		{
			Logger::WriteMessage("Test case: ScriptClass ******************************\n");
			Test("script_class.txt", "", "x:3\ny:0.14\nc:true\nd:false\ng:10\nh:false\n");
		}
	};
}
