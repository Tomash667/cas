#pragma once

namespace cas
{
	namespace internal
	{
		template<typename To, typename From>
		To union_cast(const From& f)
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

		struct CtorDtorHelper
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

			template<typename T>
			static void Destroy(T& item)
			{
				item.~T();
			}

			template<typename T>
			static void DestroyNew(T* item)
			{
				delete item;
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

			template<typename C, typename R, typename... A>
			static R ret(R(C::*)(A...) const)
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
		virtual ~RefCounter() {}
		void AddRef() { ++refs; }
		void Release() { if(--refs == 0) delete this; }
		int GetRefs() const { return refs; }
	private:
		int refs;
	};

	struct FunctionInfo
	{
		void* ptr;
		bool thiscall, builtin, return_pointer_or_reference;

		template<typename T>
		FunctionInfo(T f)
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
		FunctionInfo(nullptr_t)
		{
			builtin = true;
		}
	};
}

#define AsFunction(name, result, args) cas::FunctionInfo(static_cast<result (*) args>(name))
#define AsMethod(type, name, result, args) cas::FunctionInfo(static_cast<result (type::*) args>(&type::name))
