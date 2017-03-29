#pragma once

#include "cas/Common.h"

namespace cas
{
	class IMember;

	class IObject
	{
	public:
		virtual void AddRef() = 0;
		virtual Type GetType() = 0;
		virtual Value GetMemberValue(IMember* member) = 0;
		virtual Value GetMemberValueRef(IMember* member) = 0;
		virtual void* GetPtr() = 0;
		virtual Value GetValue() = 0;
		virtual Value GetValueRef() = 0;
		virtual bool IsGlobal() = 0;
		virtual void Release() = 0;

		template<typename T>
		T GetMemberValue(IMember* member) = delete;

		template<>
		bool GetMemberValue(IMember* member)
		{
			auto val = GetMemberValue(member);
			assert(val.type.generic_type == GenericType::Bool && !val.type.is_ref);
			return val.bool_value;
		}

		template<>
		char GetMemberValue(IMember* member)
		{
			auto val = GetMemberValue(member);
			assert(val.type.generic_type == GenericType::Char && !val.type.is_ref);
			return val.char_value;
		}

		template<>
		int GetMemberValue(IMember* member)
		{
			auto val = GetMemberValue(member);
			assert(val.type.generic_type == GenericType::Int && !val.type.is_ref);
			return val.int_value;
		}

		template<>
		float GetMemberValue(IMember* member)
		{
			auto val = GetMemberValue(member);
			assert(val.type.generic_type == GenericType::Float && !val.type.is_ref);
			return val.float_value;
		}

		template<typename T>
		T GetMemberValueRef(IMember* member) = delete;

		template<>
		bool& GetMemberValueRef(IMember* member)
		{
			auto val = GetMemberValueRef(member);
			assert(val.type.generic_type == GenericType::Bool && val.type.is_ref);
			return *(bool*)val.int_value;
		}

		template<>
		char& GetMemberValueRef(IMember* member)
		{
			auto val = GetMemberValueRef(member);
			assert(val.type.generic_type == GenericType::Char && val.type.is_ref);
			return *(char*)val.int_value;
		}

		template<>
		int& GetMemberValueRef(IMember* member)
		{
			auto val = GetMemberValueRef(member);
			assert(val.type.generic_type == GenericType::Int && val.type.is_ref);
			return *(int*)val.int_value;
		}

		template<>
		float& GetMemberValueRef(IMember* member)
		{
			auto val = GetMemberValueRef(member);
			assert(val.type.generic_type == GenericType::Float && val.type.is_ref);
			return *(float*)val.int_value;
		}

		template<typename T>
		T& Cast()
		{
			void* ptr = GetPtr();
			return *(T*)ptr;
		}

		template<typename T>
		T GetValue() = delete;

		template<>
		bool GetValue()
		{
			auto val = GetValue();
			assert(val.type.generic_type == GenericType::Bool && !val.type.is_ref);
			return val.bool_value;
		}

		template<>
		char GetValue()
		{
			auto val = GetValue();
			assert(val.type.generic_type == GenericType::Char && !val.type.is_ref);
			return val.char_value;
		}

		template<>
		int GetValue()
		{
			auto val = GetValue();
			assert(val.type.generic_type == GenericType::Int && !val.type.is_ref);
			return val.int_value;
		}

		template<>
		float GetValue()
		{
			auto val = GetValue();
			assert(val.type.generic_type == GenericType::Float && !val.type.is_ref);
			return val.float_value;
		}

		template<typename T>
		T& GetValueRef() = delete;

		template<>
		bool& GetValueRef()
		{
			auto val = GetValueRef();
			assert(val.type.generic_type == GenericType::Bool && val.type.is_ref);
			return *(bool*)val.int_value;
		}

		template<>
		char& GetValueRef()
		{
			auto val = GetValueRef();
			assert(val.type.generic_type == GenericType::Char && val.type.is_ref);
			return *(char*)val.int_value;
		}

		template<>
		int& GetValueRef()
		{
			auto val = GetValueRef();
			assert(val.type.generic_type == GenericType::Int && val.type.is_ref);
			return *(int*)val.int_value;
		}

		template<>
		float& GetValueRef()
		{
			auto val = GetValueRef();
			assert(val.type.generic_type == GenericType::Float && val.type.is_ref);
			return *(float*)val.int_value;
		}
	};
}
