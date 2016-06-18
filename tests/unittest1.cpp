#include "stdafx.h"
#include "CppUnitTest.h"

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

unsigned __stdcall ThreadStart(void* data)
{
	PackedData* pdata = (PackedData*)data;
	bool result = ParseAndRun(pdata->input, pdata->optimize);
	return (result ? 1u : 0u);
}

namespace tests
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		wstring GetWC(cstring s)
		{
			const size_t len = strlen(s);
			wstring str;
			str.resize(len);
			mbstowcs((wchar_t*)str.data(), s, len);
			return str;
		}

		Result ParseAndRunWithTimeout(cstring content, bool optimize, int timeout = 1)
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
			if(input[0] != 0)
			{
				Logger::WriteMessage("Script input:\n");
				Logger::WriteMessage(input);
				Logger::WriteMessage("\n\n");
			}

			string path(Format("../../../cases/%s", filename));
			std::ifstream ifs(path);
			Assert::IsTrue(ifs.is_open(), GetWC(Format("Failed to open file '%s'.", path.c_str())).c_str());
			std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			std::ostringstream oss;
			std::streambuf* old_cout = std::cout.rdbuf(oss.rdbuf());

			std::istringstream iss(input);
			std::streambuf* old_cin = std::cin.rdbuf(iss.rdbuf());

			Result result = ParseAndRunWithTimeout(content.c_str(), optimize);
			string s = oss.str();
			cstring ss = s.c_str();
			if(result == TIMEOUT)
			{
				cstring output = Format("Script execution/parsing timeout. Output:\n%s", ss);
				Assert::Fail(GetWC(output).c_str());
			}
			else if(result == FAILED)
			{
				cstring output = Format("Script parsing failed. Output:\n%s", ss);
				Assert::Fail(GetWC(output).c_str());
			}
			Logger::WriteMessage("Script output:\n");
			Logger::WriteMessage(ss);
			Assert::AreEqual(output, ss, "Invalid output.");

			std::cout.rdbuf(old_cout);
			std::cin.rdbuf(old_cin);
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
			Test("type_func.txt", "", "4\n");
		}

		TEST_METHOD(Block)
		{
			Logger::WriteMessage("Test case: Block ******************************\n");
			Test("block.txt", "", "4\n3\n11\n");
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
	};
}
