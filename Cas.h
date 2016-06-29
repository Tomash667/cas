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
	bool AddType(cstring type_name, int size);
	bool AddMember(cstring type_name, cstring decl, int offset);
	void RegisterStandardLib();
};

// helper
cstring Format(cstring fmt, ...);
