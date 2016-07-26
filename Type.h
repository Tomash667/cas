#pragma once

struct Function;
struct CommonFunction;
struct ParseFunction;
struct Type;

// code or script function
struct AnyFunction
{
	union
	{
		Function* f;
		ParseFunction* pf;
		CommonFunction* cf;
	};
	bool is_parse;

	inline AnyFunction(std::nullptr_t) : cf(nullptr), is_parse(false) {}
	inline AnyFunction(Function* f) : f(f), is_parse(false) {}
	inline AnyFunction(ParseFunction* pf) : pf(pf), is_parse(true) {}
	inline operator bool() const { return cf != nullptr; }
};

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
	//V_REF,
	V_SPECIAL,
	V_CLASS
};

// type
struct Type
{
	string name;
	vector<Function*> funcs;
	vector<ParseFunction*> ufuncs;
	vector<Member*> members;
	int size, index;
	bool pod, have_ctor;

	~Type();
	AnyFunction FindFunction(const string& name);
	AnyFunction FindEqualFunction(Function& fc);
	Member* FindMember(const string& name, int& index);
	static Type* Find(cstring name);
};

extern vector<Type*> types;
extern uint builtin_types;
