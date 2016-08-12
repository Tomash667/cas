#pragma once

typedef const char* cstring;

namespace cas
{
	class IModule;

	enum EventType
	{
		Info,
		Warning,
		Error
	};

	typedef void(*EventHandler)(EventType event_type, cstring msg);

	struct Settings
	{
		void* input;
		void* output;
		bool use_getch;
	};

	IModule* CreateModule();
	void DestroyModule(IModule* module);
	void Initialize(Settings* settings = nullptr);
	void Shutdown();
	void SetHandler(EventHandler handler);
};

// helper
cstring Format(cstring fmt, ...);

#include "IModule.h"
