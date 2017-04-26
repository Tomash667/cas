#pragma once

#include "cas/Common.h"
#include "cas/FunctionInfo.h"

namespace cas
{
	enum TypeFlags
	{
		ValueType = 1 << 0, // type is struct
		Complex = 1 << 1, // complex types are returned in memory
		DisallowCreate = 1 << 2, // can't create in script
		RefCount = 1 << 3 // type use addref/release operator
	};

	class IFunction;
	class IMember;
	class IModule;

	class IType
	{
	public:
		GenericType GetGenericType() const { return generic_type; }
		bool IsSimple() const
		{
			return generic_type == GenericType::Void
				|| generic_type == GenericType::Bool
				|| generic_type == GenericType::Char
				|| generic_type == GenericType::Int
				|| generic_type == GenericType::Float;
		}
		bool IsEnum() const
		{
			return generic_type == GenericType::Enum;
		}

		virtual IType* GetChildType(cstring name) = 0;
		virtual const vector<std::pair<string, int>>& GetEnumValues() = 0;
		virtual cstring GetFullName() = 0;
		virtual IMember* GetMember(cstring name) = 0;
		virtual IFunction* GetMethod(cstring name_or_decl, int flags = 0) = 0;
		virtual void GetMethodsList(vector<IFunction*>& funcs, cstring name, int flags = 0) = 0;
		virtual IModule* GetModule() = 0;
		virtual cstring GetName() = 0;
		virtual IType* GetParentType() = 0;
		virtual void QueryChildTypes(delegate<bool(IType*)> pred) = 0;
		virtual void QueryMembers(delegate<bool(IMember*)> pred) = 0;
		virtual void QueryMethods(delegate<bool(IFunction*)> pred) = 0;

	protected:
		GenericType generic_type;
	};

	class IClass : public virtual IType
	{
	public:
		virtual bool AddMember(cstring decl, int offset) = 0;
		virtual bool AddMethod(cstring decl, const FunctionInfo& func_info) = 0;

		template<typename T, typename... Args>
		bool AddCtor(cstring decl)
		{
			FunctionInfo info = (generic_type == GenericType::Struct) ?
				FunctionInfo(internal::CtorDtorHelper::Create<T, Args...>) : FunctionInfo(internal::CtorDtorHelper::CreateNew<T, Args...>);
			return AddMethod(decl, info);
		}

		template<typename T>
		bool AddDtor()
		{
			FunctionInfo info = (generic_type == GenericType::Struct) ?
				FunctionInfo(internal::CtorDtorHelper::Destroy<T>) : FunctionInfo(internal::CtorDtorHelper::DestroyNew<T>);
			return AddMethod(Format("~%s()", GetName()), info);
		}
	};

	template<typename T>
	class ISpecificClass : public IClass
	{
	public:
		template<typename... Args>
		bool AddCtor(cstring decl)
		{
			return IClass::AddCtor<T, Args...>(decl);
		}

		bool AddDtor()
		{
			return IClass::AddDtor<T>();
		}
	};

	class IEnum : public virtual IType
	{
	public:
		struct Item
		{
			cstring name;
			int value;
		};

		template<typename T>
		struct EnumClassItem
		{
			cstring name;
			T value;

			static_assert(sizeof(T) == sizeof(int), "T must be size of int.");
		};

		virtual bool AddValue(cstring name) = 0;
		virtual bool AddValue(cstring name, int value) = 0;
		virtual bool AddValues(std::initializer_list<cstring> const& items) = 0;
		virtual bool AddValues(std::initializer_list<Item> const& items) = 0;

		template<typename T>
		bool AddEnums(std::initializer_list<EnumClassItem<T>> const& items)
		{
			return AddValues((std::initializer_list<Item> const&)items);
		}
	};
}
