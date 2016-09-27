#pragma once

namespace cas
{
	struct ReturnValue
	{
		enum Type
		{
			Void,
			Bool,
			Int,
			Float
		};

		Type type;
		union
		{
			bool bool_value;
			int int_value;
			float float_value;
		};
	};
}
