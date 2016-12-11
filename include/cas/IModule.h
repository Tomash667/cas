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
		struct is_complex
		{
			static const bool value = !(std::is_trivially_default_constructible<T>::value && std::is_trivially_destructible<T>::value);
		};

		struct AsCtorHelper
		{
			template<typename T, typename... Args>
			static T Create(Args... args)
			{
				return T(args...);
			}
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
		bool thiscall, builtin;

		template<typename T>
		inline FunctionInfo(T f)
		{
			static_assert(internal::is_function_pointer<T>::value
				|| std::is_member_function_pointer<T>::value,
				"T must be function or member function pointer.");
			static_assert(sizeof(T) == sizeof(void*), "Invalid function pointer size, virtual functions unsupported yet.");
			ptr = internal::union_cast<void*>(f);
			thiscall = std::is_member_function_pointer<T>::value;
			builtin = false;
		}

		template<>
		inline FunctionInfo(nullptr_t)
		{
			builtin = true;
		}
	};

#define AsFunction(name, result, args) FunctionInfo(static_cast<result (*) args>(name))
#define AsMethod(type, name, result, args) FunctionInfo(static_cast<result (type::*) args>(&type::name))

	template<typename T, typename... Args>
	inline FunctionInfo AsCtor()
	{
		return FunctionInfo(internal::AsCtorHelper::Create<T, Args...>);
	}

	enum TypeFlags
	{
		Ref = 1 << 0, // not implemented
		Complex = 1 << 1, // complex types are returned in memory
		DisallowCreate = 1 << 2, // not implemented
		NoRefCount = 1 << 3 // not implemented
	};
	
	class IModule
	{
	public:
		enum ExecutionResult
		{
			ParsingError,
			Exception,
			Ok
		};

		virtual bool AddFunction(cstring decl, const FunctionInfo& func_info) = 0;
		virtual bool AddMethod(cstring type_name, cstring decl, const FunctionInfo& func_info) = 0;
		virtual bool AddType(cstring type_name, int size, int flags = 0) = 0;
		virtual bool AddMember(cstring type_name, cstring decl, int offset) = 0;
		virtual ReturnValue GetReturnValue() = 0;
		virtual cstring GetException() = 0;
		virtual ExecutionResult ParseAndRun(cstring input, bool optimize = true, bool decompile = false) = 0;
		virtual bool Verify() = 0;

		template<typename T>
		inline bool AddType(cstring type_name, int flags = 0)
		{
			if(internal::is_complex<T>::value)
				flags |= Complex;
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
