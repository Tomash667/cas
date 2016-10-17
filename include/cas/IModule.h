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

		template<typename T>
		struct has_constructor
		{
			static const bool value = std::is_default_constructible<T>::value && !std::is_trivially_default_constructible<T>::value;
		};

		template<typename T>
		struct has_destructor
		{
			static const bool value = std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value;
		};

		template<typename T>
		struct has_assignment_operator
		{
			static const bool value = std::is_copy_assignable<T>::value && !std::is_trivially_copy_assignable<T>::value;
		};

		template<typename T>
		struct has_copy_constructor
		{
			static const bool value = std::is_copy_constructible<T>::value && !std::is_trivially_copy_constructible<T>::value;
		};

		template<typename T>
		struct is_pod
		{
			static const bool value = !(has_constructor<T>::value
				|| has_destructor<T>::value
				|| has_assignment_operator<T>::value
				|| has_copy_constructor<T>::value);
		};
	}

	class RefCounter
	{
	public:
		RefCounter() : refs(1) {}
		inline void AddRef() { ++refs; }
		inline void Release() { if(--refs == 0) delete this; }
	private:
		int refs;
	};

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
		Ref = 1 << 0, // not implemented
		Pod = 1 << 1,
		DisallowCreate = 1 << 2, // not implemented
		NoRefCount = 1 << 3 // not implemented
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
		virtual bool Verify() = 0;

		template<typename T>
		inline bool AddType(cstring type_name, int flags = 0)
		{
			if(internal::is_pod<T>::value)
				flags |= Pod;
			return AddType(type_name, sizeof(T), flags);
		}

		template<typename T>
		inline bool AddRefType(cstring type_name, int flags = 0)
		{
			flags |= Ref;
			static_assert(std::is_base_of<RefCounter, T>::value, "AddRefType can only be used for classes derived from RefCounter.");
			bool ok = AddType<T>(type_name, flags);
			if(ok)
			{
				AddMethod(type_name, "void operator addref()", &T::AddRef);
				AddMethod(type_name, "void operator release()", &T::Release);
			}
			return ok;
		}

	protected:
		virtual ~IModule() {}
	};
}
