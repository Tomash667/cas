#pragma once

#include "cas/Common.h"

namespace cas
{
	class IModule;

	class IGlobal
	{
	public:
		virtual IModule* GetModule() = 0;
		virtual cstring GetName() = 0;
		virtual ComplexType GetType() = 0;
	};
}
