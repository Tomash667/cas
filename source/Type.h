#pragma once

#include "cas/IModule.h"

class Module;
struct Enum;
struct Function;
struct CommonFunction;
struct Member;
struct ParseFunction;
struct Type;
struct UserFunction;
enum SpecialFunction;

// code or script function
struct AnyFunction
{
	enum Type
	{
		NONE,
		CODE,
		PARSE,
		SCRIPT
	};

	union
	{
		Function* f;
		ParseFunction* pf;
		CommonFunction* cf;
		UserFunction* uf;
	};
	Type type;

	inline AnyFunction(std::nullptr_t) : cf(nullptr), type(NONE) {}
	inline AnyFunction(Function* f) : f(f), type(CODE) {}
	inline AnyFunction(ParseFunction* pf) : pf(pf), type(PARSE) {}
	inline AnyFunction(UserFunction* uf) : uf(uf), type(SCRIPT) {}
	inline AnyFunction(CommonFunction* cf, Type type) : cf(cf), type(type) {}
	inline operator bool() const { return type != NONE; }
	inline bool IsCode() const { return type == CODE; }
	inline bool IsParse() const { return type == PARSE; }
	inline bool IsScript() const { return type == SCRIPT; }
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
struct Type : public cas::IClass, public cas::IEnum
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
		RefCount = 1 << 8,
		BuiltinCtor
	};

	Module* module;
	string name;
	vector<Function*> funcs;
	vector<ParseFunction*> ufuncs;
	AnyFunction dtor;
	vector<Member*> members;
	Enum* enu;
	int size, index, flags;
	uint first_line, first_charpos;
	bool declared, built;

	inline Type() : enu(nullptr), dtor(nullptr) {}
	~Type();
	Member* FindMember(const string& name, int& index);
	Function* FindCodeFunction(cstring name);
	Function* FindSpecialCodeFunction(SpecialFunction special);
	void SetGenericType();

	inline bool IsClass() const { return IS_SET(flags, Type::Class); }
	inline bool IsRef() const { return IS_SET(flags, Type::Ref); }
	inline bool IsStruct() const { return IsClass() && !IsRef(); }
	inline bool IsRefClass() const { return IsClass() && IsRef(); }
	inline bool IsPassByValue() const { return IS_SET(flags, Type::PassByValue); }
	inline bool IsSimple() const { return ::IsSimple(index); }
	inline bool IsEnum() const { return enu != nullptr; }
	inline bool IsBuiltin() const { return IsSimple() || index == V_STRING || IsEnum(); }

	bool AddMember(cstring decl, int offset) override;
	bool AddMethod(cstring decl, const cas::FunctionInfo& func_info) override;
	bool AddValue(cstring name) override;
	bool AddValue(cstring name, int value) override;
	bool AddValues(std::initializer_list<cstring> const& items) override;
	bool AddValues(std::initializer_list<Item> const& items) override;
	cstring GetName() const override;
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
