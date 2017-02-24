#pragma once

struct Class;
struct Str;

// Call context reference to other variable or object
struct RefVar
{
#ifdef CHECK_LEAKS
	static vector<RefVar*> all_refs;
#endif

	enum Type
	{
		LOCAL,
		GLOBAL,
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
	};
	bool is_valid, to_release, ref_to_class;

	RefVar(Type type, uint index, int var_index = -999, uint depth = 0);
	~RefVar();

	bool Release();
};
