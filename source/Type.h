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
enum CoreVarType
{
	V_VOID,
	V_BOOL,
	V_CHAR,
	V_INT,
	V_FLOAT,
	V_STRING,
	V_ARRAY,
	V_REF,
	V_SPECIAL
};

enum SpecialVarType
{
	SV_NORMAL,
	//SV_CONST,
	SV_REF,
	//SV_CONST_REF,
	//SV_PTR,
	//SV_CONST_PTR
};

struct VarType
{
	int core;
	SpecialVarType special;

	VarType() {}
	explicit VarType(CoreVarType core) : core(core), special(SV_NORMAL) {}
	explicit VarType(int type, SpecialVarType special = SV_NORMAL) : core(type), special(special) {}

	inline bool operator == (const VarType& type) const
	{
		return core == type.core && special == type.special;
	}

	inline bool operator != (const VarType& type) const
	{
		return core != type.core || special != type.special;
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
	Function* FindSpecialFunction(int type);

	inline bool IsClass() const { return IS_SET(flags, Type::Class); }
	inline bool IsRef() const { return IS_SET(flags, Type::Ref); }
	inline bool IsStruct() const { return IsClass() && !IsRef(); }
};

struct Var;

struct BaseArray
{
	int refs;
	Type* type;

	virtual ~BaseArray() {}

	virtual void Add(int item) = 0;
	virtual void Clear() = 0;
	virtual uint Count() = 0;
	virtual Var Get(uint index) = 0;
	virtual void Insert(uint index, int item) = 0;
	virtual void Remove(uint index) = 0;
	virtual void Set(uint index, Var& v) = 0;

	inline void Release()
	{
		if(--refs == 0)
			delete this;
	}
};
