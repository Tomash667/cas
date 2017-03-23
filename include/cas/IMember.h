#pragma once

#include "cas/Common.h"

namespace cas
{
	class IModule;
	class IType;

	class IMember
	{
	public:
		virtual IType* GetClass() = 0;
		virtual IModule* GetModule() = 0;
		virtual cstring GetName() = 0;
		virtual uint GetOffset() = 0;
		virtual Type GetType() = 0;
	};
}
