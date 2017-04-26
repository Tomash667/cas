#include "Pch.h"
#include "CallContext.h"
#include "Decompiler.h"
#include "Engine.h"
#include "EventLogger.h"
#include "Module.h"

// cin/cout
#include <iostream>

static Engine* g_engine;
Logger* logger;

void InitLib(Module& module, cas::Settings& settings);

void Event(cas::EventType event_type, cstring msg)
{
	g_engine->HandleEvent(event_type, msg);
}

void AssertEventHandler(cstring msg, cstring file, uint line)
{
#ifdef _DEBUG
	if(IsDebuggerPresent())
		DebugBreak();
#endif
	cstring formatted = Format("Assert failed in '%s(%u)', expression '%s'.", file, line, msg);
	g_engine->HandleEvent(cas::EventType::Assert, formatted);
}

cas::IEngine* cas::IEngine::Create()
{
	if(g_engine)
		return nullptr;
	return new Engine;
}

Engine::Engine() : core_module(nullptr), handler(nullptr), initialized(false)
{
	g_engine = this;
}

Engine::~Engine()
{
	for(auto it = all_modules.rbegin(), end = all_modules.rend(); it != end; ++it)
		delete *it;
	delete logger;
	g_engine = nullptr;
}

cas::IModule* Engine::CreateModule(cas::IModule* parent_module, cstring name)
{
	assert(initialized);
	if(!parent_module)
		parent_module = core_module;
	Module* module = new Module(all_modules.size(), name);
	module->AddParentModule(parent_module);
	all_modules.push_back(module);
	return module;
}

cas::EventHandler Engine::GetHandler()
{
	return handler;
}

bool Engine::Initialize(cas::Settings* new_settings)
{
	if(initialized)
		return false;

	cas::Settings settings;
	settings.input = &std::cin;
	settings.output = &std::cout;
	if(new_settings)
		settings = *new_settings;
	if(settings.use_assert_handler)
		set_assert_handler(AssertEventHandler);
	if(settings.use_logger_handler)
		logger = new EventLogger;
	else
		logger = new NullLogger;

	core_module = new Module(0, "Core");
	all_modules.push_back(core_module);

	have_errors = false;
	InitLib(*core_module, settings);
	if(have_errors)
	{
		delete core_module;
		return false;
	}

	Decompiler::Get().Init(settings);

	initialized = true;
	return true;
}

void Engine::Release()
{
	delete this;
}

void Engine::SetHandler(cas::EventHandler new_handler)
{
	handler = new_handler;
}

Engine& Engine::Get()
{
	assert(g_engine);
	return *g_engine;
}

void Engine::HandleEvent(cas::EventType event_type, cstring msg)
{
	if(event_type == cas::EventType::Error)
		have_errors = true;
	if(handler)
		handler(event_type, msg);
}

void Engine::RemoveModule(Module* module)
{
	assert(module);
	RemoveElement(all_modules, module);
}
