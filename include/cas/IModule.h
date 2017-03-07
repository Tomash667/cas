#pragma once

#include "cas/FunctionInfo.h"
#include "cas/IType.h"

namespace cas
{
	class ICallContext;
	class IFunction;

	enum GetFlags
	{
		ByDecl = 1 << 0,
		IgnoreParent = 1 << 1
	};

	class IModule
	{
	public:
		enum ParseResult
		{
			ValidationError,
			ParsingError,
			Ok
		};

		struct Options
		{
			bool optimize;

			Options() : optimize(true) {}
		};

		virtual IEnum* AddEnum(cstring type_name) = 0;
		virtual bool AddFunction(cstring decl, const FunctionInfo& func_info) = 0;
		virtual bool AddParentModule(IModule* module) = 0;
		virtual IClass* AddType(cstring type_name, int size, int flags = 0) = 0;
		virtual ICallContext* CreateCallContext(cstring name = nullptr) = 0;
		virtual void Decompile() = 0;
		virtual IFunction* GetFunction(cstring name_or_decl, int flags = 0) = 0;
		virtual cstring GetName() = 0;
		virtual ParseResult Parse(cstring input) = 0;
		virtual void Release() = 0;
		virtual void Reset() = 0;
		virtual void SetName(cstring name) = 0;
		virtual void SetOptions(const Options& options) = 0;

		template<typename T>
		ISpecificClass<T>* AddType(cstring type_name, int flags = 0)
		{
			if(internal::is_complex<T>::value)
				flags |= Complex;
			return (ISpecificClass<T>*)AddType(type_name, sizeof(T), flags);
		}

		template<typename T>
		ISpecificClass<T>* AddRefType(cstring type_name, int flags = 0)
		{
			static_assert(std::is_base_of<RefCounter, T>::value, "AddRefType can only be used for classes derived from RefCounter.");
			flags |= RefCount;
			IClass* type = AddType<T>(type_name, flags);
			if(type)
			{
				type->AddMethod("void operator addref()", &T::AddRef);
				type->AddMethod("void operator release()", &T::Release);
			}
			return (ISpecificClass<T>*)type;
		}
		
	protected:
		~IModule() {}
	};
}
