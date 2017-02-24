#pragma once

#include "cas/Common.h"
#include "cas/ReturnValue.h"

namespace cas
{
	class ICallContext
	{
	public:
		virtual ~ICallContext() {}

		virtual std::pair<cstring, int> GetCurrentLocation() = 0;
		virtual cstring GetException() = 0;
		virtual cstring GetName() = 0;
		virtual ReturnValue GetReturnValue() = 0;
		virtual bool Run() = 0;
		virtual void SetName(cstring name) = 0;
	};
}
