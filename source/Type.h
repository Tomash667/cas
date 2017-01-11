#pragma once

struct Enum;
struct Function;
struct CommonFunction;
struct Member;
struct ParseFunction;
struct Type;
enum SpecialFunction;

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
	V_ARRAY,
	V_REF,
	V_SPECIAL,
	V_TYPE,
	V_MAX
};

inline bool IsSimple(int type) { return type == V_BOOL || type == V_CHAR || type == V_INT || type == V_FLOAT; }

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
	enum Flags
	{
		Ref = 1 << 0,
		Complex = 1 << 1, // complex types are returned in memory
		DisallowCreate = 1 << 2,
		NoRefCount = 1 << 3,
		Hidden = 1 << 4,
		Class = 1 << 5,
		Code = 1 << 6,
		PassByValue = 1 << 7, // struct/string
		RefCount = 1 << 8
	};

	string name;
	vector<Function*> funcs;
	vector<ParseFunction*> ufuncs;
	vector<Member*> members;
	Enum* enu;
	int size, index, flags;
	uint first_line, first_charpos;
	bool declared, built;

	inline Type() : enu(nullptr) {}
	~Type();
	Member* FindMember(const string& name, int& index);
	Function* FindCodeFunction(cstring name);
	Function* FindSpecialCodeFunction(SpecialFunction special);

	inline bool IsClass() const { return IS_SET(flags, Type::Class); }
	inline bool IsRef() const { return IS_SET(flags, Type::Ref); }
	inline bool IsStruct() const { return IsClass() && !IsRef(); }
	inline bool IsRefClass() const { return IsClass() && IsRef(); }
	inline bool IsPassByValue() const { return IS_SET(flags, Type::PassByValue); }
	inline bool IsSimple() const { return ::IsSimple(index); }
	inline bool IsEnum() const { return enu != nullptr; }
	inline bool IsBuiltin() const { return IsSimple() || index == V_STRING || IsEnum(); }
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
	union
	{
		bool bvalue;
		char cvalue;
		int value;
		float fvalue;
	};
	enum UsedMode
	{
		No,
		Used,
		UsedBeforeSet,
		Set
	};
	UsedMode used;
	bool have_def_value;
};

struct Enum
{
	Type* type;
	vector<std::pair<string, int>> values;

	std::pair<string, int>* Find(const string& id)
	{
		for(auto& val : values)
		{
			if(val.first == id)
				return &val;
		}
		return nullptr;
	}
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
