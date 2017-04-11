#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

const int DEFAULT_TIMEOUT = (CI_MODE ? 60 : 1);

IEngine* engine;
IModule* current_module;
istringstream s_input;
ostringstream s_output;
string event_output, content;
int reg_errors;

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
	engine = engine->Create();

	engine->SetHandler(TestEventHandler);
	Settings s;
	s.input = &s_input;
	s.output = &s_output;
	s.use_getch = false;
	s.use_assert_handler = !IsDebuggerPresent();
	s.use_debuglib = true;
	s.decompile_marker = true;
	if(!engine->Initialize(&s))
		Assert::IsTrue(event_output.empty(), L"Cas initialization failed.");

	if(CI_MODE)
		Logger::WriteMessage("+++ CI MODE +++\n\n");
	else
		Logger::WriteMessage("+++ NORMAL MODE +++\n\n");
}

TEST_MODULE_CLEANUP(ModuleCleanup)
{
	engine->Release();
}

void WriteDecompileOutput(IModule* module)
{
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

Result RunTestImpl(const TestSettings& settings)
{
	auto options = settings.module->GetOptions();
	options.optimize = settings.optimize;
	settings.module->SetOptions(options);

	Result result;
	bool can_decompile = false;
	ICallContext* call_context = nullptr;

	try
	{
		// parse
		bool ok = true;
		if(settings.input)
		{
			IModule::ParseResult parse_result = settings.module->Parse(settings.input);
			if(parse_result == IModule::Ok)
			{
				if(settings.decompile)
					WriteDecompileOutput(settings.module);
			}
			else
				ok = false;
		}
		else
			can_decompile = true;

		// run
		if(ok)
		{
			call_context = settings.module->CreateCallContext();
			if(!call_context->Run())
			{
				event_output = Format("Exception: %s", call_context->GetException());
				call_context->Release();
				call_context = nullptr;
				result = FAILED;
			}
			else
			{
				vector<string>& asserts = call_context->GetAsserts();
				if(!asserts.empty())
				{
					event_output.clear();
					event_output = Format("Asserts failed (%u):\n", asserts.size());
					for(string& s : asserts)
					{
						event_output += s;
						event_output += "\n";
					}
					call_context->Release();
					call_context = nullptr;
					result = ASSERT;
				}
				else
				{
					if(settings.ret_delegate)
					{
						Retval retval(call_context);
						settings.ret_delegate(retval);
					}
					call_context->Release();
					call_context = nullptr;
					result = OK;
				}
			}
		}
		else
			result = FAILED;
	}
	catch(cstring)
	{
		if(call_context)
			call_context->Release();
		if(can_decompile && settings.decompile)
			WriteDecompileOutput(settings.module);
		result = ASSERT;
	}

	return result;
}

unsigned __stdcall RunTestThread(void* data)
{
	const TestSettings& settings = *(const TestSettings*)data;
	return RunTestImpl(settings);
}

Result RunTestSite(const TestSettings& settings, int timeout = DEFAULT_TIMEOUT)
{
	if(IsDebuggerPresent())
		return RunTestImpl(settings);

	HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0u, RunTestThread, (void*)&settings, 0u, nullptr);
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

void RunTest(TestSettings& settings)
{
	// verify module registeration before RunTest
	Assert::AreEqual(0, reg_errors, L"Test registeration failed.");

	// clear event
	event_output.clear();

	// write input
	if(!CI_MODE && settings.input && settings.input[0] != 0)
	{
		Logger::WriteMessage("\nScript input:\n");
		Logger::WriteMessage(settings.input);
		Logger::WriteMessage("\n");
	}

	// reset io
	s_input.clear();
	s_input.str(settings.input ? settings.input : "");
	s_output.clear();
	s_output.str("");

	// get script code
	if(settings.filename)
	{
		string path(Format("../cases/%s", settings.filename));
		ifstream ifs(path);
		Assert::IsTrue(ifs.is_open(), GetWC(Format("Failed to open file '%s'.", path.c_str())).c_str());
		content = string((istreambuf_iterator<char>(ifs)), (istreambuf_iterator<char>()));
		ifs.close();
		settings.input = content.c_str();
	}
	else
		settings.input = settings.code;

	// run
	Result result = RunTestSite(settings);
	
	// get output
	string output = s_output.str();
	cstring ss = output.c_str();

	// check result
	switch(result)
	{
	case OK:
		if(settings.error)
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
		if(!settings.error)
		{
			cstring output = Format("Script parsing failed. Parse output:\n%s\nOutput: %s", event_output.c_str(), ss);
			Assert::Fail(GetWC(output).c_str());
		}
		else
			AssertError(settings.error);
		break;
	}

	// verify output
	if(settings.output)
	{
		if(!CI_MODE)
		{
			Logger::WriteMessage("\nScript output:\n");
			Logger::WriteMessage(ss);
			Logger::WriteMessage("\n");
		}

		Assert::AreEqual(settings.output, ss, "Invalid output.");
	}

	// cleanup
	if(!settings.dont_reset)
		settings.module->Reset();
	reg_errors = 0;
	event_output.clear();
}

void CleanupErrors()
{
	event_output.clear();
	reg_errors = 0;
}

void CleanupOutput()
{
	s_output.clear();
	s_output.str("");
}

void AssertError(cstring error, const __LineInfo* pLineInfo)
{
	cstring r = strstr(event_output.c_str(), error);
	if(!r)
		Assert::Fail(GetWC(Format("Invalid error message. Expected:<%s> Actual:<%s>", error, event_output.c_str())).c_str(), pLineInfo);
}

void AssertOutput(cstring expected, const __LineInfo* pLineInfo)
{
	string output = s_output.str();
	cstring ss = output.c_str();
	Assert::AreEqual(expected, ss, "Invalid output.", pLineInfo);
}

void WriteOutput(cstring msg)
{
	s_output << msg;
}
