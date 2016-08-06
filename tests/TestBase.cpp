#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

const int DEFAULT_TIMEOUT = (CI_MODE ? 60 : 1);
std::istringstream s_input;
std::ostringstream s_output;
string event_output;

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

void TestEventHandler(cas::EventType event_type, cstring msg)
{
	cstring type;
	switch(event_type)
	{
	case cas::Info:
		type = "INFO";
		break;
	case cas::Warning:
		type = "WARN";
		break;
	case cas::Error:
	default:
		type = "ERROR";
		break;
	}
	cstring m = Format("%s: %s\n", type, msg);
	Logger::WriteMessage(m);
	event_output += m;
}

TEST_MODULE_INITIALIZE(ModuleInitialize)
{
	cas::SetHandler(TestEventHandler);
	cas::Settings s;
	s.input = &s_input;
	s.output = &s_output;
	s.use_getch = false;
	cas::Initialize(&s);
	Assert::IsTrue(event_output.empty(), L"Cas initialization failed.");

	if(CI_MODE)
		Logger::WriteMessage("+++ CI MODE +++\n\n");
	else
		Logger::WriteMessage("+++ NORMAL MODE +++\n\n");
}

TEST_MODULE_CLEANUP(ModuleCleanup)
{
}

unsigned __stdcall ThreadStart(void* data)
{
	PackedData* pdata = (PackedData*)data;
	bool result = cas::ParseAndRun(pdata->input, pdata->optimize);
	return (result ? 1u : 0u);
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

wstring GetWC(cstring s)
{
	const size_t len = strlen(s);
	wstring str;
	str.resize(len);
	mbstowcs((wchar_t*)str.data(), s, len);
	return str;
}

void RunFileTest(cstring filename, cstring input, cstring output, bool optimize)
{
	event_output.clear();

	if(!CI_MODE && input[0] != 0)
	{
		Logger::WriteMessage("\nScript input:\n");
		Logger::WriteMessage(input);
		Logger::WriteMessage("\n");
	}

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

	if(!CI_MODE)
	{
		Logger::WriteMessage("\nScript output:\n");
		Logger::WriteMessage(ss);
		Logger::WriteMessage("\n");
	}

	Assert::AreEqual(output, ss, "Invalid output.");
}

void RunFailureTest(cstring code, cstring error)
{
	event_output.clear();
	
	s_input.clear();
	s_input.str("");
	s_output.clear();
	s_output.str("");

	Result result = ParseAndRunWithTimeout(code, true);
	string s = s_output.str();
	cstring ss = s.c_str();
	if(result == TIMEOUT)
		Assert::Fail(L"Failure timeout.");
	else if(result == FAILED)
	{
		cstring r = strstr(event_output.c_str(), error);
		if(!r)
			Assert::Fail(GetWC(Format("Invalid error message. Expected:<%s> Actual:<%s>", error, event_output.c_str())).c_str());
	}
	else
		Assert::Fail(L"Failure without error.");
}