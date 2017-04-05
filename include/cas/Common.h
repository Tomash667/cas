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

	enum class GenericType
	{
		Void,
		Bool,
		Char,
		Int,
		Float,
		String,
		Class,
		Struct,
		Enum,
		Invalid
	};

	struct Type
	{
		GenericType generic_type;
		IType* specific_type; // only for enum or object
		bool is_ref;

		Type() {}
		Type(GenericType generic_type, IType* specific_type = nullptr, bool is_ref = false) : generic_type(generic_type), specific_type(specific_type),
			is_ref(is_ref) {}
	};

	template<typename T>
	using delegate = ssvu::FastFunc<T>;
}
