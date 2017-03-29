#pragma once

struct Class;
struct Global;
struct Str;

// Call context reference to other variable or object
struct RefVar
{
	enum Type
	{
		LOCAL,
		GLOBAL,
		CGLOBAL,
		MEMBER,
		INDEX,
		CODE
	};

	Type type;
	int refs, var_index, value;
	uint index, depth;
	union
	{
		Class* clas;
		Str* str;
		int* adr;
		Global* global;
	};
	bool is_valid, to_release, ref_to_class;

	RefVar(Type type, uint index, int var_index = -999, uint depth = 0);
	~RefVar();

	bool Release();
};
