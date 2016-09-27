#pragma once

struct AssertException {};

class TestEnvironment : public testing::Environment
{
public:
	TestEnvironment();
	static TestEnvironment& Get() { return *env; }

	void SetUp() override;
	void TearDown() override;

	void Init();
	void ResetIO(cstring input = nullptr);
	void Log(cstring msg, EventType type);

	static void Info(cstring msg) { Get().Log(msg, EventType::Info); }
	static void Warn(cstring msg) { Get().Log(msg, EventType::Warning); }
	static void Error(cstring msg) { Get().Log(msg, EventType::Error); }

	static const bool CI_MODE = ((_CI_MODE - 1) == 0);
	static const int DEFAULT_TIMEOUT = (CI_MODE ? 60 : 1);

	istringstream s_input;
	ostringstream s_output;
	string event_output;
	IModule* current_module;
	bool decompile;

private:
	static TestEnvironment* env;
};
