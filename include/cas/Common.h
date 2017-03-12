#pragma once

#include <vector>
#include <string>
#include "FastFunc.h"

using std::string;
using std::vector;

typedef const char* cstring;
typedef unsigned int uint;

inline uint alignto(uint size, uint to)
{
	uint n = size / to;
	if(size % to != 0)
		++n;
	return n * to;
}

cstring Format(cstring msg, ...);

namespace cas
{
	class IType;

	struct Value
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

	struct ComplexType
	{
		IType* type;
		bool ref;
	};

	template<typename T>
	using delegate = ssvu::FastFunc<T>;
}
