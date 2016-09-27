#include "Pch.h"
#include "TestEnvironment.h"

TestEnvironment* TestEnvironment::env;

namespace testing
{
	namespace internal
	{
		enum GTestColor {
			COLOR_DEFAULT,
			COLOR_RED,
			COLOR_GREEN,
			COLOR_YELLOW
		};

		extern void ColoredPrintf(GTestColor color, const char* fmt, ...);
	}
}
#define PRINTF(...)  do { testing::internal::ColoredPrintf(testing::internal::COLOR_GREEN, "[          ] "); testing::internal::ColoredPrintf(testing::internal::COLOR_YELLOW, __VA_ARGS__); } while(0)


void TestEventHandler(EventType event_type, cstring msg)
{
	TestEnvironment& env = TestEnvironment::Get();
	env.Log(msg, event_type);

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

	env.event_output += Format("%s: %s\n", type, msg);
	if(event_type == EventType::Assert)
		throw AssertException();
}

TestEnvironment::TestEnvironment() : decompile(false), current_module(nullptr)
{
	env = this;
}

void TestEnvironment::SetUp()
{
	SetHandler(TestEventHandler);
	Settings s;
	s.input = &s_input;
	s.output = &s_output;
	s.use_getch = false;
	s.use_assert_handler = true;
	bool ok = false;
	ASSERT_TRUE(Initialize(&s)) << "Cas initialization failed.";
	
	if(CI_MODE)
		Info("+++ CI MODE +++");
	else
		Info("+++ NORMAL MODE +++");
}

void TestEnvironment::TearDown()
{
	Shutdown();
}

void TestEnvironment::Init()
{
	DebugBreak();
}

void TestEnvironment::ResetIO(cstring input)
{
	event_output.clear();

	s_input.clear();
	s_input.str(input ? input : "");
	s_output.clear();
	s_output.str("");
}

void TestEnvironment::Log(cstring msg, EventType event_type)
{
	assert(msg);

	testing::internal::GTestColor color;
	cstring type;

	switch(event_type)
	{
	case EventType::Info:
		type = "[   INFO   ] ";
		color = testing::internal::COLOR_DEFAULT;
		break;
	case EventType::Warning:
		type = "[   WARN   ] ";
		color = testing::internal::COLOR_YELLOW;
		break;
	case EventType::Error:
	default:
		type = "[   ERROR  ] ";
		color = testing::internal::COLOR_RED;
		break;
	case EventType::Assert:
		type = "[  ASSERT  ] ";
		color = testing::internal::COLOR_RED;
		break;
	}
	testing::internal::ColoredPrintf(color, type);
	testing::internal::ColoredPrintf(testing::internal::COLOR_DEFAULT, msg);
	testing::internal::ColoredPrintf(testing::internal::COLOR_DEFAULT, "\n");
}
