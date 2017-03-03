#pragma once

#include "cas/Settings.h"

namespace cas
{
	class ICallContext;
	class IModule;
	
	class IEngine
	{
	public:
		static IEngine* Create();

		virtual IModule* CreateModule(IModule* parent_module = nullptr, cstring name = nullptr) = 0;
		virtual EventHandler GetHandler() = 0;
		virtual bool Initialize(Settings* settings = nullptr) = 0;
		virtual void Release() = 0;
		virtual void SetHandler(EventHandler handler) = 0;

	protected:
		~IEngine() {}
	};
}
