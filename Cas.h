#pragma once

typedef const char* cstring;

namespace cas
{
	enum EventType
	{
		Warning,
		Error
	};

	typedef void(*EventHandler)(EventType event_type, cstring msg);

	void Initialize();
	void SetHandler(EventHandler handler);
	bool ParseAndRun(cstring input, bool optimize = true, bool decompile = false);
	bool AddFunction(cstring decl, void* ptr);
	bool AddMethod(cstring type_name, cstring decl, void* ptr);
	bool AddType(cstring type_name, int size, bool pod);
	bool AddMember(cstring type_name, cstring decl, int offset);

	template<typename T>
	inline bool AddType(cstring type_name)
	{
		bool hasConstructor = std::is_default_constructible<T>::value && !std::is_trivially_default_constructible<T>::value;
		bool hasDestructor = std::is_destructible<T>::value && !std::is_trivially_destructible<T>::value;
		bool hasAssignmentOperator = std::is_copy_assignable<T>::value && !std::is_trivially_copy_assignable<T>::value;
		bool hasCopyConstructor = std::is_copy_constructible<T>::value && !std::is_trivially_copy_constructible<T>::value;
		return AddType(type_name, sizeof(T), !(hasConstructor || hasDestructor || hasAssignmentOperator || hasCopyConstructor));
	}
};

// helper
cstring Format(cstring fmt, ...);
