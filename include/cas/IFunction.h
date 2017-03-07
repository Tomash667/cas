#pragma once

#include "cas/Common.h"

namespace cas
{
	class IType;

	class IFunction
	{
	public:
		/*enum Flags
		{
			F_THISCALL = 1 << 0,
			F_IMPLICIT = 1 << 1,
			F_BUILTIN = 1 << 2,
			F_DELETE = 1 << 3,
			F_CODE = 1 << 4,
			F_STATIC = 1 << 5,
			F_DEFAULT = 1 << 6
		};

		virtual int GetArgCount() = 0;
		virtual IType* GetArgType(int index) = 0;
		virtual Value GetArgDefaultValue(int index) = 0;
		virtual cstring GetArgName(int index) = 0;
		virtual cstring GetDecl() = 0;
		virtual int GetFlags() = 0;*/
		virtual cstring GetName() = 0;
		/*virtual IType* GetReturnType() = 0;
		virtual IType* GetType() = 0;*/
	};
}
