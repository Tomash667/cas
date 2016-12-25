#pragma once

#include <vector>
#include <string>

typedef const char* cstring;

namespace cas
{
	class IModule;

	enum class EventType
	{
		Info,
		Warning,
		Error,
		Assert
	};

	typedef void(*EventHandler)(EventType event_type, cstring msg);

	struct Settings
	{
		void* input;
		void* output;
		bool use_corelib;
		bool use_debuglib;
		bool use_getch;
		bool use_assert_handler;
		bool decompile_marker;

		Settings() : input(nullptr), output(nullptr), use_getch(true), use_corelib(true), use_debuglib(true), use_assert_handler(true), decompile_marker(false)
		{

		}

		inline void operator = (const Settings& s)
		{
			if(s.input)
				input = s.input;
			if(s.output)
				output = s.output;
			use_corelib = s.use_corelib;
			use_debuglib = s.use_debuglib;
			use_getch = s.use_getch;
			use_assert_handler = s.use_assert_handler;
			decompile_marker = s.decompile_marker;
		}
	};

	bool Initialize(Settings* settings = nullptr);
	void Shutdown();
	void SetHandler(EventHandler handler);
	IModule* CreateModule();
	void DestroyModule(IModule* module);

	// Debug functions
	std::vector<std::string>& GetAsserts();
	std::pair<cstring, int> GetCurrentLocation();
};

// helper
cstring Format(cstring fmt, ...);

#include "cas/IModule.h"
