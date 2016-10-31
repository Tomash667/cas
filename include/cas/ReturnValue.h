#pragma once

namespace cas
{
	struct ReturnValue
	{
		enum Type
		{
			Void,
			Bool,
			Char,
			Int,
			Float
		};

		Type type;
		union
		{
			bool bool_value;
			char char_value;
			int int_value;
			float float_value;
		};
	};
}
