#pragma once

#include "cas/Common.h"
#include "cas/IEngine.h"

class Module;

// Script engine
class Engine : public cas::IEngine
{
public:
	Engine();
	~Engine();

	cas::ICallContext* CreateCallContext(cas::IModule* module, cstring name, bool ignoreParsed) override;
	cas::IModule* CreateModule(cas::IModule* parent_module, cstring name) override;
	cas::EventHandler GetHandler() override;
	bool Initialize(cas::Settings* settings) override;
	bool Release() override;
	void SetHandler(cas::EventHandler handler) override;

	static Engine& Get()
	{
		assert(g_engine);
		return *g_engine;
	}
	void HandleEvent(cas::EventType event_type, cstring msg);

private:
	static Engine* g_engine;
	cas::EventHandler handler;
	Module* core_module;
	Logger* logger;
	int refs, module_counter, callcontext_counter;
	bool have_errors, initialized;
};
