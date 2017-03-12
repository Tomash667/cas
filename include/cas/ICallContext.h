#pragma once

#include "cas/Common.h"

namespace cas
{
	class IGlobal;
	class IObject;
	class IModule;

	class ICallContext
	{
	public:
		virtual vector<string>& GetAsserts() = 0;
		virtual std::pair<cstring, int> GetCurrentLocation() = 0;
		virtual cstring GetException() = 0;
		virtual IObject* GetGlobal(IGlobal* global) = 0;
		virtual IModule* GetModule() = 0;
		virtual cstring GetName() = 0;
		virtual Value GetReturnValue() = 0;
		virtual void Release() = 0;
		virtual bool Run() = 0;
		virtual void SetName(cstring name) = 0;

	protected:
		~ICallContext() {}
	};
}
