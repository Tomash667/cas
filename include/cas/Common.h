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

	struct Type
	{
		GenericType generic_type;
		IType* specific_type; // only for enum or object
		bool is_ref;

		Type() {}
		Type(GenericType generic_type, IType* specific_type = nullptr) : generic_type(generic_type), specific_type(specific_type), is_ref(false) {}
	};

	struct Value
	{
		Type type;
		union
		{
			bool bool_value;
			char char_value;
			int int_value;
			float float_value;
			cstring str_value;
			IObject* obj;
		};

		Value() {}
		Value(bool bool_value) : type(GenericType::Bool), bool_value(bool_value) {}
		Value(char char_value) : type(GenericType::Char), char_value(char_value) {}
		Value(int int_value) : type(GenericType::Int), int_value(int_value) {}
		Value(float float_value) : type(GenericType::Float), float_value(float_value) {}
		Value(cstring str_value) : type(GenericType::String), str_value(str_value) {}
		Value(IObject* obj) : type(GenericType::Object), obj(obj) {}

		template<typename T>
		Value(const T&) = delete;
	};

	template<typename T>
	using delegate = ssvu::FastFunc<T>;
}
