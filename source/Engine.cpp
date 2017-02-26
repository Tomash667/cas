#include "Pch.h"
#include "CallContext.h"
#include "Decompiler.h"
#include "Engine.h"
#include "EventLogger.h"
#include "Module.h"

// cin/cout
#include <iostream>

static Engine* g_engine;

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

Engine::Engine() : core_module(nullptr), handler(nullptr), logger(nullptr), module_counter(0), callcontext_counter(0), initialized(false)
{
	g_engine = this;
}

Engine::~Engine()
{
	delete core_module;
	delete logger;
	g_engine = nullptr;
}

cas::ICallContext* Engine::CreateCallContext(cas::IModule* module, cstring name, bool ignore_parsed)
{
	assert(initialized);
	assert(module);
	CallContext* call_context = new CallContext(callcontext_counter++, (Module&)*module, name);
	return call_context;
}

cas::IModule* Engine::CreateModule(cas::IModule* parent_module, cstring name)
{
	assert(initialized);
	if(!parent_module)
		parent_module = core_module;
	Module* module = new Module(module_counter++, name);
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

	core_module = new Module(0, nullptr);

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

bool Engine::Release()
{
	delete this;
	return true;
}

void Engine::SetHandler(cas::EventHandler new_handler)
{
	handler = new_handler;
}

void Engine::HandleEvent(cas::EventType event_type, cstring msg)
{
	if(event_type == cas::EventType::Error)
		have_errors = true;
	if(handler)
		handler(event_type, msg);
}

/*
IModule* cas::CreateModule()
{
	assert(initialized);
	if(!initialized)
		return nullptr;

	Module* module = new Module(module_index, core_module);
	++module_index;

	return module;
}

void cas::DestroyModule(IModule* _module)
{
	Module* module = (Module*)_module;

	assert(initialized);
	assert(module && !module->released);
	if(!initialized || !module || module->released)
		return;

	module->RemoveRef(true);
}

*/