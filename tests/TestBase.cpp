#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

const int DEFAULT_TIMEOUT = (CI_MODE ? 60 : 1);
istringstream s_input;
ostringstream s_output;
string event_output, content;
IModule* current_module;
int reg_errors;
bool decompile, reset_parser = true, first_run;

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
		++reg_errors;
		break;
	case EventType::Assert:
		type = "ASSERT";
		++reg_errors;
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
	s.use_debuglib = true;
	s.decompile_marker = true;
	if(!Initialize(&s))
		Assert::IsTrue(event_output.empty(), L"Cas initialization failed.");

	if(CI_MODE)
		Logger::WriteMessage("+++ CI MODE +++\n\n");
	else
		Logger::WriteMessage("+++ NORMAL MODE +++\n\n");
}

TEST_MODULE_CLEANUP(ModuleCleanup)
{
	Shutdown();
}

void WriteDecompileOutput(IModule* module)
{
	if(!decompile)
		return;

	module->Decompile();

	cstring mark = "***DCMP***";
	cstring mark_end = "***DCMP_END***";
	string s = s_output.str();
	size_t pos = s.find(mark, 0);
	if(pos != string::npos)
	{
		size_t end = s.find(mark_end, pos);
		size_t len = strlen(mark);
		string decomp;
		if(end != string::npos)
		{
			decomp = s.substr(pos + len, end - pos - len);
			s.erase(pos, end - pos + strlen(mark_end));
		}
		else
		{
			decomp = s.substr(pos + len);
			s.erase(pos, s.size() - pos);
		}
		Logger::WriteMessage(decomp.c_str());
		s_output.clear();
		s_output.str(s);
	}
}

Result ParseAndRunChecked(IModule* module, cstring input, bool optimize)
{
	IModule::Options options;
	options.optimize = optimize;
	module->SetOptions(options);

	bool can_decompile = false;
	Result result;
	try
	{
		IModule::ExecutionResult ex_result;
		if(input)
		{
			ex_result = module->Parse(input);
			if(ex_result == IModule::Ok)
			{
				WriteDecompileOutput(module);
				ex_result = module->Run();
			}
		}
		else
		{
			can_decompile = true;
			ex_result = module->Run();
		}

		if(ex_result != IModule::Ok)
		{
			if(ex_result == IModule::Exception)
				event_output = Format("Exception: %s", module->GetException());
			result = FAILED;
		}
		else
		{
			vector<string>& asserts = GetAsserts();
			if(!asserts.empty())
			{
				event_output.clear();
				event_output = Format("Asserts failed (%u):\n", asserts.size());
				for(string& s : asserts)
				{
					event_output += s;
					event_output += "\n";
				}
				result = ASSERT;
			}
			else
				result = OK;
		}
	}
	catch(cstring)
	{
		if(can_decompile)
			WriteDecompileOutput(module);
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
	if(reset_parser && content && !first_run)
		module->ResetParser();
	first_run = false;

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

void RunTest(const TestSettings& s)
{
	// verify module registeration before RunTest
	Assert::AreEqual(0, reg_errors, L"Test registeration failed.");

	// clear event
	event_output.clear();

	// write input
	if(!CI_MODE && s.input && s.input[0] != 0)
	{
		Logger::WriteMessage("\nScript input:\n");
		Logger::WriteMessage(s.input);
		Logger::WriteMessage("\n");
	}

	// reset io
	s_input.clear();
	s_input.str(s.input ? s.input : "");
	s_output.clear();
	s_output.str("");

	// get script code
	cstring code;
	if(s.filename)
	{
		string path(Format("../cases/%s", s.filename));
		ifstream ifs(path);
		Assert::IsTrue(ifs.is_open(), GetWC(Format("Failed to open file '%s'.", path.c_str())).c_str());
		content = string((istreambuf_iterator<char>(ifs)), (istreambuf_iterator<char>()));
		ifs.close();
		code = content.c_str();
	}
	else
		code = s.code;

	// run
	Result result = ParseAndRunWithTimeout(s.module, code, s.optimize);
	
	// get output
	string output = s_output.str();
	cstring ss = output.c_str();

	// check result
	switch(result)
	{
	case OK:
		if(s.error)
			Assert::Fail(L"Failure without error.");
		break;
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
		if(!s.error)
		{
			cstring output = Format("Script parsing failed. Parse output:\n%s\nOutput: %s", event_output.c_str(), ss);
			Assert::Fail(GetWC(output).c_str());
		}
		else
			AssertError(s.error);
		break;
	}

	// verify output
	if(s.output)
	{
		if(!CI_MODE)
		{
			Logger::WriteMessage("\nScript output:\n");
			Logger::WriteMessage(ss);
			Logger::WriteMessage("\n");
		}

		Assert::AreEqual(s.output, ss, "Invalid output.");
	}

	// cleanup
	if(reset_parser && code)
		s.module->ResetParser();
	reg_errors = 0;
	event_output.clear();
}

void CleanupErrors()
{
	event_output.clear();
	reg_errors = 0;
}

void CleanupAsserts()
{
	GetAsserts().clear();
}

void AssertError(cstring error)
{
	cstring r = strstr(event_output.c_str(), error);
	if(!r)
		Assert::Fail(GetWC(Format("Invalid error message. Expected:<%s> Actual:<%s>", error, event_output.c_str())).c_str());
}

void SetDecompile(bool _decompile)
{
	decompile = _decompile;
}

void SetResetParser(bool _reset_parser)
{
	reset_parser = _reset_parser;
}

void WriteOutput(cstring msg)
{
	s_output << msg;
}
