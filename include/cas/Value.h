#pragma once

#include "cas/IType.h"

namespace cas
{
	class IObject;

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
			string* str_ptr;
			IObject* obj;
		};

		Value() {}
		Value(bool bool_value) : type(GenericType::Bool), bool_value(bool_value) {}
		Value(char char_value) : type(GenericType::Char), char_value(char_value) {}
		Value(int int_value) : type(GenericType::Int), int_value(int_value) {}
		Value(float float_value) : type(GenericType::Float), float_value(float_value) {}
		Value(cstring str_value) : type(GenericType::String), str_value(str_value) {}
		Value(string* str_ptr) : type(GenericType::String, nullptr, true), str_ptr(str_ptr) {}
		Value(string& str_ref) : type(GenericType::String, nullptr, true), str_ptr(&str_ref) {}
		Value(IObject* obj);
		Value(Type&& type, int value) : type(type), int_value(value) {}

		static Value Enum(IType* type, const int& value)
		{
			assert(type->IsEnum());
			return Value(Type(GenericType::Enum, type), value);
		}

		template<typename T>
		static Value Enum(IType* type, const T& value)
		{
			static_assert(std::is_enum<T>::value, "T must be enum");
			assert(type->IsEnum());
			return Value(Type(GenericType::Enum, type), (int)value);
		}

		static Value Enum(IType* type, int* ptr)
		{
			assert(type->IsEnum());
			return Value(Type(GenericType::Enum, type, true), (int)ptr);
		}

		template<typename T>
		static Value Enum(IType* type, T* ptr)
		{
			static_assert(std::is_enum<T>::value, "T must be enum");
			assert(type->IsEnum());
			return Value(Type(GenericType::Enum, type, true), (int)ptr);
		}

		template<typename T>
		Value(const T&) = delete;
	};
}
