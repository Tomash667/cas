#pragma once

#include "cas/Common.h"

namespace cas
{
	class IType;

	struct ReturnValue
	{
		IType* type;
		union
		{
			bool bool_value;
			char char_value;
			int int_value;
			float float_value;
			cstring str_value;
		};
	};
}
