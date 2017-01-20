#pragma once

#include "cas/Exception.h"

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
	V_COMPLEX,
	V_GENERIC,
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

	inline int GetBaseType() const
	{
		assert(type != V_COMPLEX);
		if(type == V_REF)
			return subtype;
		else
			return type;
	}

	inline int GetBaseSubtype(int generic_type) const
	{
		assert(type != V_COMPLEX);
		int result;
		if(type == V_REF || type == V_ARRAY)
			result = subtype;
		else
			result = type;
		if(result == V_GENERIC)
		{
			assert(generic_type != V_VOID);
			return generic_type;
		}
		else
			return result;
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
		RefCount = 1 << 8,
		Generic = 1 << 9
	};

	string name;
	vector<Function*> funcs;
	vector<ParseFunction*> ufuncs;
	vector<Member*> members;
	string generic_param;
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
	inline bool IsGeneric() const { return IS_SET(flags, Type::Generic); }
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

struct Array
{
	int refs;
	Type* type;
	vector<byte> data;

	inline Array(Type* type) : refs(1), type(type)
	{

	}

	inline void add(int* item)
	{
		uint offset = data.size();
		data.resize(type->size);
		memcpy(data.data() + offset, item, type->size);
	}

	inline byte* at(int index)
	{
		if(index < 0 || index > count())
			throw CasException(Format("Index %d out of range.", index));
		return data.data() + index * type->size;
	}

	inline byte* back(int offset = 0)
	{
		return at(count() - offset - 1);
	}

	inline void clear()
	{
		data.clear();
	}

	inline int count() const
	{
		return data.size() / type->size;
	}

	inline bool empty() const
	{
		return data.empty();
	}

	inline byte* front(int offset = 0)
	{
		return at(offset);
	}

	inline void insert(int index, int* item)
	{
		if(index < 0 || index > count())
			throw CasException(Format("Index %d out of range.", index));
		data.resize(data.size() + type->size);
		memmove(data.data() + (index + 1) * type->size, data.data() + index * type->size, data.size() - index * type->size);
		memcpy(data.data() + index * type->size, item, type->size);
	}

	inline void pop()
	{
		if(data.empty())
			throw CasException("Array empty.");
		data.resize(data.size() - type->size);
	}

	inline void remove(int index)
	{
		if(index < 0 || index > count())
			throw CasException(Format("Index %d out of range.", index));
		memmove(data.data() + index * type->size, data.data() + (index + 1) * type->size, data.size() - (index - 1) * type->size);
		data.resize(data.size() - type->size);
	}

	inline void resize(int size)
	{
		if(size < 0)
			throw CasException(Format("Invalid array size %d.", size));
		data.resize(size * type->size);
	}

	inline byte* operator [] (int index)
	{
		return at(index);
	}

	inline bool operator == (Array* arr)
	{
		assert(type == arr->type);
		if(this == arr)
			return true;
		else if(count() == arr->count())
			return memcmp(data.data(), arr->data.data(), data.size() * type->size) == 0;
		else
			return false;
	}

	inline bool operator != (Array* arr)
	{
		return !(operator == (arr));
	}

	inline void operator += (int* item)
	{
		add(item);
	}

	inline void operator += (Array& arr)
	{
		assert(type == arr.type);
		uint offset = data.size();
		uint size = arr.data.size();
		data.resize(offset + arr.data.size());
		memcpy(data.data() + offset, arr.data.data(), size);
	}

	inline void Release()
	{
		if(--refs == 0)
			delete this;
	}
};
