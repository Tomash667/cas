#pragma once

struct Function;
struct CommonFunction;
struct Member;
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
	int refs, seed;

	inline void Release()
	{
		--refs;
		if(refs == 0)
			Free();
	}
};

// var type
enum CoreVarType
{
	V_VOID,
	V_BOOL,
	V_CHAR,
	V_INT,
	V_FLOAT,
	V_STRING,
	V_REF,
	V_SPECIAL,
	V_TYPE,
	V_MAX
};

struct VarType
{
	int type, subtype;

	VarType() {}
	VarType(nullptr_t) : type(0), subtype(0) {}
	VarType(CoreVarType core) : type(core), subtype(0) {}
	VarType(int type, int subtype) : type(type), subtype(subtype) {}

	inline bool operator == (const VarType& vartype) const
	{
		return type == vartype.type && subtype == vartype.subtype;
	}

	inline bool operator != (const VarType& vartype) const
	{
		return type != vartype.type || subtype != vartype.subtype;
	}

	inline int GetType() const
	{
		if(type == V_REF)
			return subtype;
		else
			return type;
	}
};

// type
struct Type
{
	// must be compatibile with TypeFlags
	enum Flags
	{
		Ref = 1 << 0,
		Complex = 1 << 1, // complex types are returned in memory
		DisallowCreate = 1 << 2,
		NoRefCount = 1 << 3,
		HaveCtor = 1 << 4,
		Hidden = 1 << 5,
		Class = 1 << 6
	};

	string name;
	vector<Function*> funcs;
	vector<ParseFunction*> ufuncs;
	vector<Member*> members;
	int size, index, flags;

	~Type();
	Member* FindMember(const string& name, int& index);

	inline bool IsClass() const { return IS_SET(flags, Type::Class); }
	inline bool IsRef() const { return IS_SET(flags, Type::Ref); }
	inline bool IsStruct() const { return IsClass() && !IsRef(); }
};

struct VarSource
{
	int index;
	bool mod;
};

// class member
struct Member : public VarSource
{
	string name;
	VarType vartype;
	int offset;
};
