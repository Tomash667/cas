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

		template<typename T>
		struct is_pointer_or_reference
		{
			static const bool value = (std::is_pointer<T>::value || std::is_reference<T>::value);
		};

		struct AsCtorHelper
		{
			template<typename T, typename... Args>
			static T Create(Args... args)
			{
				return T(args...);
			}

			template<typename T, typename... Args>
			static T* CreateNew(Args... args)
			{
				return new T(args...);
			}
		};

		template<typename T>
		struct function_traits
		{
		private:
			template<typename R, typename... A>
			static R ret(R(*)(A...))
			{
				return R();
			}

			template<typename C, typename R, typename... A>
			static R ret(R(C::*)(A...))
			{
				return R();
			}

		public:
			typedef typename decltype(ret(T())) result;
		};
	}

	class RefCounter
	{
	public:
		RefCounter() : refs(1) {}
		inline void AddRef() { ++refs; }
		inline void Release() { if(--refs == 0) delete this; }
		inline int GetRefs() const { return refs; }
	private:
		int refs;
	};

	struct FunctionInfo
	{
		void* ptr;
		bool thiscall, builtin, return_pointer_or_reference;

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
			return_pointer_or_reference = internal::is_pointer_or_reference<internal::function_traits<T>::result>::value;
		}

		template<>
		inline FunctionInfo(nullptr_t)
		{
			builtin = true;
		}
	};

#define AsFunction(name, result, args) FunctionInfo(static_cast<result (*) args>(name))
#define AsMethod(type, name, result, args) FunctionInfo(static_cast<result (type::*) args>(&type::name))

	enum TypeFlags
	{
		ValueType = 1 << 0, // type is struct
		Complex = 1 << 1, // complex types are returned in memory
		DisallowCreate = 1 << 2, // can't create in script
		RefCount = 1 << 3 // type use addref/release operator
	};

	class IType
	{
	public:
		virtual bool AddMember(cstring decl, int offset) = 0;
		virtual bool AddMethod(cstring decl, const FunctionInfo& func_info) = 0;

		template<typename T, typename... Args>
		inline bool AddCtor(cstring decl)
		{
			FunctionInfo info = is_struct ?
				FunctionInfo(internal::AsCtorHelper::Create<T, Args...>) : FunctionInfo(internal::AsCtorHelper::CreateNew<T, Args...>);
			return AddMethod(decl, info);
		}

	protected:
		bool is_struct;
	};

	template<typename T>
	class ISpecificType : public IType
	{
	public:
		template<typename... Args>
		inline bool AddCtor(cstring decl)
		{
			return IType::AddCtor<T, Args...>(decl);
		}
	};

	class IModule
	{
	public:
		enum ExecutionResult
		{
			ValidationError,
			ParsingError,
			Exception,
			Ok
		};

		virtual bool AddFunction(cstring decl, const FunctionInfo& func_info) = 0;
		virtual IType* AddType(cstring type_name, int size, int flags = 0) = 0;
		virtual ReturnValue GetReturnValue() = 0;
		virtual cstring GetException() = 0;
		virtual ExecutionResult ParseAndRun(cstring input, bool optimize = true, bool decompile = false) = 0;

		template<typename T>
		inline ISpecificType<T>* AddType(cstring type_name, int flags = 0)
		{
			if(internal::is_complex<T>::value)
				flags |= Complex;
			return (ISpecificType<T>*)AddType(type_name, sizeof(T), flags);
		}

		template<typename T>
		inline ISpecificType<T>* AddRefType(cstring type_name, int flags = 0)
		{
			flags |= RefCount;
			static_assert(std::is_base_of<RefCounter, T>::value, "AddRefType can only be used for classes derived from RefCounter.");
			IType* type = AddType<T>(type_name, flags);
			if(type)
			{
				type->AddMethod("void operator addref()", &T::AddRef);
				type->AddMethod("void operator release()", &T::Release);
			}
			return (ISpecificType<T>*)type;
		}

	protected:
		virtual ~IModule() {}
	};
}
