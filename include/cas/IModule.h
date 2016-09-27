#pragma once

#include "cas/ReturnValue.h"

namespace cas
{
	class IModule
	{
	public:
		virtual bool AddFunction(cstring decl, void* ptr) = 0;
		virtual bool AddMethod(cstring type_name, cstring decl, void* ptr) = 0;
		virtual bool AddType(cstring type_name, int size, bool pod) = 0;
		virtual bool AddMember(cstring type_name, cstring decl, int offset) = 0;
		virtual ReturnValue GetReturnValue() = 0;
		virtual bool ParseAndRun(cstring input, bool optimize = true, bool decompile = false) = 0;

		template<typename T>
		inline bool AddType(cstring type_name)
		{
			bool hasConstructor = std::is_default_constructible<T>::value && !std::is_trivially_default_constructible<T>::value;
			bool hasDestructor = std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value;
			bool hasAssignmentOperator = std::is_copy_assignable<T>::value && !std::is_trivially_copy_assignable<T>::value;
			bool hasCopyConstructor = std::is_copy_constructible<T>::value && !std::is_trivially_copy_constructible<T>::value;
			return AddType(type_name, sizeof(T), !(hasConstructor || hasDestructor || hasAssignmentOperator || hasCopyConstructor));
		}

	protected:
		virtual ~IModule() {}
	};
}
