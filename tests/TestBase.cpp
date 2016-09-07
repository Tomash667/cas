#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

const int DEFAULT_TIMEOUT = (CI_MODE ? 60 : 1);
std::istringstream s_input;
std::ostringstream s_output;
string event_output;
IModule* def_module;

enum Result
{
	OK,
	TIMEOUT,
	FAILED,
	ASSERT
};

struct PackedData
{
	IModule* module;
	cstring input;
	bool optimize;
};

wstring GetWC(cstring s)
{
	const size_t len = strlen(s);
	wstring str;
	str.resize(len);
	mbstowcs((wchar_t*)str.data(), s, len);
	return str;
}

void TestEventHandler(EventType event_type, cstring msg)
{
	cstring type;
	switch(event_type)
	{
	case EventType::Info:
		type = "INFO";
		break;
	case EventType::Warning:
		type = "WARN";
		break;
	case EventType::Error:
	default:
		type = "ERROR";
		break;
	case EventType::Assert:
		type = "ASSERT";
		break;
	}
	cstring m = Format("%s: %s\n", type, msg);
	Logger::WriteMessage(m);
	event_output += m;
	if(event_type == EventType::Assert)
		throw msg;
}

TEST_MODULE_INITIALIZE(ModuleInitialize)
{
	SetHandler(TestEventHandler);
	Settings s;
	s.input = &s_input;
	s.output = &s_output;
	s.use_getch = false;
	s.use_assert_handler = !IsDebuggerPresent();
	if(!Initialize(&s))
		Assert::IsTrue(event_output.empty(), L"Cas initialization failed.");

	def_module = CreateModule();

	if(CI_MODE)
		Logger::WriteMessage("+++ CI MODE +++\n\n");
	else
		Logger::WriteMessage("+++ NORMAL MODE +++\n\n");
}

TEST_MODULE_CLEANUP(ModuleCleanup)
{
	Shutdown();
}

Result ParseAndRunChecked(IModule* module, cstring input, bool optimize)
{
	Result result;
	try
	{
		if(!module->ParseAndRun(input, optimize))
			result = FAILED;
		else
			result = OK;
	}
	catch(cstring)
	{
		result = ASSERT;
	}
	return result;
}

unsigned __stdcall ThreadStart(void* data)
{
	PackedData* pdata = (PackedData*)data;
	return ParseAndRunChecked(pdata->module, pdata->input, pdata->optimize);
}

Result ParseAndRunWithTimeout(IModule* module, cstring content, bool optimize, int timeout = DEFAULT_TIMEOUT)
{
	if(IsDebuggerPresent())
		return ParseAndRunChecked(module, content, optimize);

	PackedData pdata;
	pdata.module = module;
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
	return (Result)exit_code;
}

void RunFileTest(IModule* module, cstring filename, cstring input, cstring output, bool optimize)
{
	event_output.clear();

	if(!CI_MODE && input[0] != 0)
	{
		Logger::WriteMessage("\nScript input:\n");
		Logger::WriteMessage(input);
		Logger::WriteMessage("\n");
	}

	string path(Format("../cases/%s", filename));
	std::ifstream ifs(path);
	Assert::IsTrue(ifs.is_open(), GetWC(Format("Failed to open file '%s'.", path.c_str())).c_str());
	std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	ifs.close();

	s_input.clear();
	s_input.str(input);
	s_output.clear();
	s_output.str("");

	Result result = ParseAndRunWithTimeout(module, content.c_str(), optimize);
	string s = s_output.str();
	cstring ss = s.c_str();
	switch(result)
	{
	case TIMEOUT:
		{
			cstring output = Format("Script execution/parsing timeout. Parse output:\n%s\nOutput: %s", event_output.c_str(), ss);
			Assert::Fail(GetWC(output).c_str());
		}
		break;
	case ASSERT:
		Assert::Fail(GetWC(event_output.c_str()).c_str());
		break;
	case FAILED:
		{
			cstring output = Format("Script parsing failed. Parse output:\n%s\nOutput: %s", event_output.c_str(), ss);
			Assert::Fail(GetWC(output).c_str());
		}
		break;
	}

	if(!CI_MODE)
	{
		Logger::WriteMessage("\nScript output:\n");
		Logger::WriteMessage(ss);
		Logger::WriteMessage("\n");
	}

	Assert::AreEqual(output, ss, "Invalid output.");
}

void RunTest(IModule* module, cstring code)
{
	event_output.clear();

	s_input.clear();
	s_input.str("");
	s_output.clear();
	s_output.str("");

	Result result = ParseAndRunWithTimeout(module, code, true);
	string s = s_output.str();
	cstring ss = s.c_str();

	switch(result)
	{
	case TIMEOUT:
		Assert::Fail(L"Test timeout.");
		break;
	case ASSERT:
		Assert::Fail(GetWC(event_output.c_str()).c_str());
		break;
	case FAILED:
		{
			cstring output = Format("Script parsing failed. Parse output:\n%s\nOutput: %s", event_output.c_str(), ss);
			Assert::Fail(GetWC(output).c_str());
		}
		break;
	}
}

void RunFailureTest(IModule* module, cstring code, cstring error)
{
	event_output.clear();
	
	s_input.clear();
	s_input.str("");
	s_output.clear();
	s_output.str("");

	Result result = ParseAndRunWithTimeout(module, code, true);
	string s = s_output.str();
	cstring ss = s.c_str();

	switch(result)
	{
	case OK:
		Assert::Fail(L"Failure without error.");
		break;
	case TIMEOUT:
		Assert::Fail(L"Failure timeout.");
		break;
	case ASSERT:
		Assert::Fail(GetWC(event_output.c_str()).c_str());
		break;
	case FAILED:
		{
			cstring r = strstr(event_output.c_str(), error);
			if(!r)
				Assert::Fail(GetWC(Format("Invalid error message. Expected:<%s> Actual:<%s>", error, event_output.c_str())).c_str());
		}
		break;
	}
		
}
