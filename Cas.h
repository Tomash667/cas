#pragma once

typedef const char* cstring;

namespace cas
{
	enum EventType
	{
		Info,
		Warning,
		Error
	};

	typedef void(*EventHandler)(EventType event_type, cstring msg);

	struct Settings
	{
		void* input;
		void* output;
		bool use_getch;
	};

	struct ReturnValue
	{
		enum Type
		{
			Void,
			Bool,
			Int,
			Float
		};

		Type type;
		union
		{
			bool bool_value;
			int int_value;
			float float_value;
		};
	};

	void Initialize(Settings* settings = nullptr);
	void SetHandler(EventHandler handler);
	bool ParseAndRun(cstring input, bool optimize = true, bool decompile = false);
	bool AddFunction(cstring decl, void* ptr);
	bool AddMethod(cstring type_name, cstring decl, void* ptr);
	bool AddType(cstring type_name, int size, bool pod);
	bool AddMember(cstring type_name, cstring decl, int offset);
	ReturnValue GetReturnValue();

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
