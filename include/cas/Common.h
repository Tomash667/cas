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
	class IObject;
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
		Object,
		Invalid
	};

	struct Value
	{
		GenericType generic_type;
		IType* type;
		union
		{
			bool bool_value;
			char char_value;
			int int_value;
			float float_value;
			cstring str_value;
			IObject* obj;
		};
		bool is_ref;

		Value() {}
		Value(bool bool_value) : generic_type(GenericType::Bool), type(nullptr), bool_value(bool_value), is_ref(false) {}
		Value(char char_value) : generic_type(GenericType::Char), type(nullptr), char_value(char_value), is_ref(false) {}
		Value(int int_value) : generic_type(GenericType::Int), type(nullptr), int_value(int_value), is_ref(false) {}
		Value(float float_value) : generic_type(GenericType::Float), type(nullptr), float_value(float_value), is_ref(false) {}
		Value(cstring str_value) : generic_type(GenericType::String), type(nullptr), str_value(str_value), is_ref(false) {}
		Value(IObject* obj) : generic_type(GenericType::Object), type(nullptr), obj(obj), is_ref(false) {}

		template<typename T>
		Value(const T&) = delete;
	};

	struct ComplexType
	{
		IType* type;
		bool ref;
	};

	template<typename T>
	using delegate = ssvu::FastFunc<T>;
}
