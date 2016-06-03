#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;

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

		void Test(cstring filename, cstring input, cstring output)
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

			bool result = ParseAndRun(content.c_str());
			string s = oss.str();
			cstring ss = s.c_str();
			Logger::WriteMessage("Script output:\n");
			Logger::WriteMessage(ss);
			Assert::IsTrue(result, L"Parsing failed.");
			Assert::AreEqual(output, ss, "Invalid output.");

			std::cout.rdbuf(old_cout);
			std::cin.rdbuf(old_cin);
		}
		
		TEST_METHOD(Simple)
		{
			Test("simple.txt", "4", "Podaj a: Odwrocone a: -4");
		}

		TEST_METHOD(Math)
		{
			Test("math.txt", "1 2 3 4", "7\n2\n3\n-2\n0\n37\n");
			Test("math.txt", "8 15 4 3", "68\n119\n23\n-120\n0\n37\n");
		}

		TEST_METHOD(Assign)
		{
			Test("assign.txt", "", "5\n1\n8\n4\n");
		}

		TEST_METHOD(String)
		{
			Test("string.txt", "Tomash 1990", "Podaj imie: Podaj rok urodzenia: Witaj Tomash! Masz 26 lat.");
		}

		TEST_METHOD(Float)
		{
			Test("float.txt", "7", "153.934\n5.5\n5.5\n3\n");
		}

		TEST_METHOD(Parentheses)
		{
			Test("parentheses.txt", "", "-5\n20\n0\n24\n");
		}

		TEST_METHOD(Bool)
		{
			Test("bool.txt", "7 8", "false\ntrue\nfalse\nfalse\ntrue\ntrue\n-1\n8\ntrue\nfalse\n");
		}

		TEST_METHOD(CompOperators)
		{
			Test("comp_operators.txt", "", "true\ntrue\ntrue\ntrue\ntrue\nfalse\nfalse\ntrue\nfalse\n");
		}

		TEST_METHOD(IfElse)
		{
			Test("if_else.txt", "1 1", "a == b");
			Test("if_else.txt", "2 1", "a > b");
			Test("if_else.txt", "1 2", "a < b");
		}
	};
}