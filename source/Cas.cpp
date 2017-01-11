#include "Pch.h"
#include "Function.h"
#include "CasImpl.h"
#include "Type.h"
#include "Module.h"

using namespace cas;

Logger* logger;
EventHandler handler;
static Module* core_module;
static int module_index;
static bool initialized;
static bool have_errors;
static bool using_event_logger;

void Event(EventType event_type, cstring msg)
{
	if(event_type == EventType::Error)
		have_errors = true;
	if(handler)
		handler(event_type, msg);
}

struct EventLogger : Logger
{
	void Log(cstring text, LOG_LEVEL level) override
	{
		EventType type;
		switch(level)
		{
		case L_INFO:
			type = EventType::Info;
			break;
		case L_WARN:
			type = EventType::Warning;
			break;
		case L_ERROR:
		default:
			type = EventType::Error;
			break;
		}
		Event(type, text);
	}
	
	void Log(cstring text, LOG_LEVEL level, const tm& time) override
	{
		Log(text, level);
	}

	void Flush() override
	{

	}
};

void AssertEventHandler(cstring msg, cstring file, uint line)
{ 
#ifdef _DEBUG
	if(IsDebuggerPresent())
		DebugBreak();
#endif
	handler(EventType::Assert, Format("Assert failed in '%s(%u)', expression '%s'.", file, line, msg));
}

bool cas::Initialize(Settings* settings)
{
	assert(!initialized);
	if(initialized)
		return false;

	Settings _settings;
	_settings.input = &cin;
	_settings.output = &cout;
	if(settings)
		_settings = *settings;
	if(_settings.use_assert_handler)
		set_assert_handler(AssertEventHandler);
	if(_settings.use_logger_handler)
	{
		logger = new EventLogger;
		using_event_logger = true;
	}

	module_index = 1;
	core_module = new Module(0, nullptr);

	have_errors = false;
	InitLib(*core_module, _settings);
	if(have_errors)
	{
		delete core_module;
		return false;
	}

	Decompiler::Get().Init(_settings);

	initialized = true;
	return true;
}

void cas::Shutdown()
{
	assert(initialized);
	if(!initialized)
		return;

	initialized = false;
	Module::all_modules_shutdown = true;
	for(Module* m : Module::all_modules)
		delete m;
	Module::all_modules.clear();
	if(using_event_logger)
		delete logger;
}

void cas::SetHandler(EventHandler _handler)
{
	handler = _handler;
}

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

Type::~Type()
{
	delete enu;
	DeleteElements(members);
}

Member* Type::FindMember(const string& name, int& index)
{
	index = 0;
	for(Member* m : members)
	{
		if(m->name == name)
			return m;
		++index;
	}
	return nullptr;
}

Function* Type::FindCodeFunction(cstring name)
{
	for(Function* f : funcs)
	{
		if(f->name == name)
			return f;
	}
	return nullptr;
}

Function* Type::FindSpecialCodeFunction(SpecialFunction special)
{
	for(Function* f : funcs)
	{
		if(f->special == special)
			return f;
	}
	return nullptr;
}

bool CommonFunction::Equal(CommonFunction& f) const
{
	if(f.arg_infos.size() != arg_infos.size())
		return false;
	for(uint i = 0, count = arg_infos.size(); i < count; ++i)
	{
		if(arg_infos[i].vartype != f.arg_infos[i].vartype)
			return false;
	}
	return true;
}
