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
			std::string content((std::istreambuf_iterator<char>(ifs)),
				(std::istreambuf_iterator<char>()));

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
	};
}