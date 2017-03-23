#pragma once

#include "cas/Common.h"
#include "cas/IEngine.h"

class CallContext;
class Module;

// Script engine
class Engine final : public cas::IEngine
{
public:
	Engine();
	~Engine();

	cas::IModule* CreateModule(cas::IModule* parent_module, cstring name) override;
	cas::EventHandler GetHandler() override;
	bool Initialize(cas::Settings* settings) override;
	void Release() override;
	void SetHandler(cas::EventHandler handler) override;

	static Engine& Get();
	void HandleEvent(cas::EventType event_type, cstring msg);
	void RemoveModule(Module* module);

private:
	cas::EventHandler handler;
	vector<Module*> all_modules;
	Module* core_module;
	bool have_errors, initialized;
};
