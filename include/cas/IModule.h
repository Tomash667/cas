#pragma once

#include "cas/ReturnValue.h"

namespace cas
{
	namespace internal
	{
		template<typename To, typename From>
		inline To union_cast(const From& f)
		{
			union
			{
				To to;
				From from;
			} a;

			a.from = f;
			return a.to;
		}

		template<typename testType>
		struct is_function_pointer
		{
			static const bool value =
				std::is_pointer<testType>::value ?
				std::is_function<typename std::remove_pointer<testType>::type>::value :
				false;
		};
	}

	struct FunctionInfo
	{
		void* ptr;
		bool thiscall;

		template<typename T>
		FunctionInfo(T f)
		{
			static_assert(internal::is_function_pointer<T>::value
				|| std::is_member_function_pointer<T>::value,
				"T must be function or member function pointer.");
			static_assert(sizeof(T) == sizeof(void*), "Invalid function pointer size, virtual functions unsupported yet.");
			ptr = internal::union_cast<void*>(f);
			thiscall = std::is_member_function_pointer<T>::value;
		}
	};

#define AsFunction(name, result, args) FunctionInfo(static_cast<result (*) args>(name))
#define AsMethod(type, name, result, args) FunctionInfo(static_cast<result (type::*) args>(&type::name))

	enum TypeFlags
	{
		Pod = 1 << 0,
		DisallowCreate = 1 << 1
	};
	
	class IModule
	{
	public:
		virtual bool AddFunction(cstring decl, const FunctionInfo& func_info) = 0;
		virtual bool AddMethod(cstring type_name, cstring decl, const FunctionInfo& func_info) = 0;
		virtual bool AddType(cstring type_name, int size, int flags = 0) = 0;
		virtual bool AddMember(cstring type_name, cstring decl, int offset) = 0;
		virtual ReturnValue GetReturnValue() = 0;
		virtual bool ParseAndRun(cstring input, bool optimize = true, bool decompile = false) = 0;

		template<typename T>
		inline bool AddType(cstring type_name, bool disallow_create = false)
		{
			bool hasConstructor = std::is_default_constructible<T>::value && !std::is_trivially_default_constructible<T>::value;
			bool hasDestructor = std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value;
			bool hasAssignmentOperator = std::is_copy_assignable<T>::value && !std::is_trivially_copy_assignable<T>::value;
			bool hasCopyConstructor = std::is_copy_constructible<T>::value && !std::is_trivially_copy_constructible<T>::value;
			bool pod = !(hasConstructor || hasDestructor || hasAssignmentOperator || hasCopyConstructor);
			int flags = 0;
			if(pod)
				flags |= Pod;
			if(disallow_create)
				flags |= DisallowCreate;
			return AddType(type_name, sizeof(T), flags);
		}

	protected:
		virtual ~IModule() {}
	};
}
