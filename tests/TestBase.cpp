#include "Pch.h"
#include "TestBase.h"

/*

#ifdef _DEBUG
_CrtMemState mem_start, mem_test_start, mem_end, mem_cmp;
unsigned total_leaks, total_leaks_size;
#endif

struct Result
{
	
	ResultId code;
	bool mem_leaks;
};



wstring GetWC(cstring s)
{
	const size_t len = strlen(s);
	wstring str;
	str.resize(len);
	mbstowcs((wchar_t*)str.data(), s, len);
	return str;
}



TEST_MODULE_CLEANUP(ModuleCleanup)
{
	Shutdown();

#ifdef _DEBUG
	_CrtMemCheckpoint(&mem_end);
	if(_CrtMemDifference(&mem_cmp, &mem_start, &mem_end))
	{
		Logger::WriteMessage(Format("%u MEMORY LEAKS detected in module cleanup (%u bytes).", mem_cmp.lCounts[_NORMAL_BLOCK], mem_cmp.lSizes[_NORMAL_BLOCK]));
		total_leaks += mem_cmp.lCounts[_NORMAL_BLOCK];
		total_leaks_size += mem_cmp.lSizes[_NORMAL_BLOCK];
	}
	if(total_leaks)
		Logger::WriteMessage(Format("TOTAL LEAKS COUNT %u (%u BYTES)!", total_leaks, total_leaks_size));
#endif
}



Result ParseAndRunWithTimeout(IModule* module, cstring content, bool optimize, int timeout = DEFAULT_TIMEOUT)
{
#ifdef _DEBUG
	_CrtMemCheckpoint(&mem_test_start);
#endif

	Result result;
	result.mem_leaks = false;
	result.code = ParseAndRunWithTimeoutInternal(module, content, optimize, timeout);

	cas::CleanupPools();

#ifdef _DEBUG
	_CrtMemCheckpoint(&mem_end);
	if(_CrtMemDifference(&mem_cmp, &mem_test_start, &mem_end))
	{
		Logger::WriteMessage(Format("%u MEMORY LEAKS detected in test cleanup (%u bytes).", mem_cmp.lCounts[_NORMAL_BLOCK], mem_cmp.lSizes[_NORMAL_BLOCK]));
		result.mem_leaks = true;
		total_leaks += mem_cmp.lCounts[_NORMAL_BLOCK];
		total_leaks_size += mem_cmp.lSizes[_NORMAL_BLOCK];
	}
#endif

	return result;
}


*/

struct PackedData
{
	TestBase* test;
	IModule* module;
	cstring input;
	bool optimize;
};

TestBase::TestBase() : env(TestEnvironment::Get())
{

}

void TestBase::SetUp()
{
	module = CreateModule();
	env.current_module = module;
}

void TestBase::TearDown()
{
	DestroyModule(module);
	env.current_module = nullptr;
}

unsigned __stdcall ThreadStart(void* data)
{
	PackedData* pdata = (PackedData*)data;
	return (unsigned)pdata->test->ParseAndRunChecked(pdata->module, pdata->input, pdata->optimize);
}

Result TestBase::ParseAndRunWithTimeout(IModule* run_module, cstring content, bool optimize, int timeout)
{
	ret.Set(run_module);

	if(IsDebuggerPresent() || timeout == 0)
		return ParseAndRunChecked(run_module, content, optimize);

	if(timeout < 0)
		timeout = env.DEFAULT_TIMEOUT;

	PackedData pdata;
	pdata.test = this;
	pdata.module = run_module;
	pdata.input = content;
	pdata.optimize = optimize;

	HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0u, ThreadStart, &pdata, 0u, nullptr);
	DWORD result = WaitForSingleObject(thread, timeout * 1000);
	EXPECT_TRUE(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT) << "Failed to create parsing thread.";
	if(result == WAIT_TIMEOUT)
	{
		TerminateThread(thread, 2);
		return Result::TIMEOUT;
	}
	else if(result != WAIT_OBJECT_0)
		return Result::FAILED;

	DWORD exit_code;
	GetExitCodeThread(thread, &exit_code);
	return (Result)exit_code;
}

Result TestBase::ParseAndRunChecked(IModule* run_module, cstring input, bool optimize)
{
	Result result;
	try
	{
		if(!module->ParseAndRun(input, optimize, env.decompile))
			result = Result::FAILED;
		else
			result = Result::OK;
	}
	catch(const AssertException&)
	{
		result = Result::ASSERT;
	}
	catch(...)
	{
		ADD_FAILURE() << "Unexpected exception caught.";
		result = Result::FAILED;
	}
	return result;
}

void TestBase::RunTest(cstring code)
{
	env.ResetIO();

	Result result = ParseAndRunWithTimeout(module, code, true);

	if(!env.CI_MODE)
	{
		string output = env.s_output.str();
		env.Info(Format("Script output: [%s]", output.c_str()));
	}

	switch(result)
	{
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
