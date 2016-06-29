#pragma once

struct Function;
struct Type;

// string implementation
struct Str : ObjectPoolProxy<Str>
{
	string s;
	int refs;

	inline void Release()
	{
		--refs;
		if(refs == 0)
			Free();
	}
};

// class member
struct Member
{
	string name;
	int type;
	int offset;
};

// var type
enum VAR_TYPE
{
	V_VOID,
	V_BOOL,
	V_INT,
	V_FLOAT,
	V_STRING,
	V_REF,
	V_SPECIAL,
	V_CLASS
};

// type
extern vector<Type*> types;
struct Type
{
	string name;
	vector<Function*> funcs;
	vector<Member*> members;
	int size, index;
	bool pod;

	Function* FindFunction(const string& name);
	inline Member* FindMember(const string& name, int& index)
	{
		index = 0;
		for(Member* m : members)
		{
			if(m->name == name)
				return m;
			++index;
		}
		return nullptr;
	}
	static inline Type* Find(cstring name)
	{
		assert(name);
		for(Type* type : types)
		{
			if(type->name == name)
				return type;
		}
		return nullptr;
	}
};
