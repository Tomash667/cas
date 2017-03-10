#pragma once

#include "cas/IType.h"
#include "AnyFunction.h"
#include "VarType.h"

class IModuleProxy;
struct Enum;
struct Member;
enum SpecialFunction;

// Script type
// Can be simple type, class, struct or enum
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

	IModuleProxy* module_proxy;
	string name;
	vector<AnyFunction> funcs;
	AnyFunction dtor;
	vector<Member*> members;
	Enum* enu;
	int size, index, flags;
	uint first_line, first_charpos;
	bool declared, built, have_def_value, have_complex_member;

	Type() : enu(nullptr), dtor(nullptr), have_def_value(false), have_complex_member(false) {}
	~Type();

	// from IType
	cstring GetName() const override;
	cas::IFunction* GetFunction(cstring name_or_decl, int flags) override;
	void GetFunctionsList(vector<cas::IFunction*>& funcs, cstring name, int flags) override;

	// from IClass
	bool AddMember(cstring decl, int offset) override;
	bool AddMethod(cstring decl, const cas::FunctionInfo& func_info) override;

	// from IEnum
	bool AddValue(cstring name) override;
	bool AddValue(cstring name, int value) override;
	bool AddValues(std::initializer_list<cstring> const& items) override;
	bool AddValues(std::initializer_list<Item> const& items) override;

	void FindAllCtors(vector<AnyFunction>& funcs);
	void FindAllFunctionOverloads(const string& name, vector<AnyFunction>& funcs);
	void FindAllStaticFunctionOverloads(const string& name, vector<AnyFunction>& funcs);
	AnyFunction FindEqualFunction(AnyFunction _f);
	AnyFunction FindFunction(const string& name);
	AnyFunction FindFunction(cstring name, delegate<bool(AnyFunction& f)> pred);
	Member* FindMember(const string& name, int& index);
	CodeFunction* FindSpecialCodeFunction(SpecialFunction special);
	AnyFunction FindSpecialFunction(SpecialFunction spec, delegate<bool(AnyFunction& f)> pred);
	void SetGenericType();

	bool IsClass() const { return IS_SET(flags, Type::Class); }
	bool IsRef() const { return IS_SET(flags, Type::Ref); }
	bool IsStruct() const { return IsClass() && !IsRef(); }
	bool IsRefClass() const { return IsClass() && IsRef(); }
	bool IsPassByValue() const { return IS_SET(flags, Type::PassByValue); }
	bool IsSimple() const { return ::IsSimple(index); }
	bool IsEnum() const { return enu != nullptr; }
	bool IsBuiltin() const { return IsSimple() || index == V_STRING || IsEnum(); }
	bool IsRefCounted() const { return IS_SET(flags, Type::RefCount); }
	bool IsAssignable() const { return IsPassByValue() || IsClass(); }
	bool IsCode() const { return IS_SET(flags, Type::Code); }
	bool IsHidden() const { return IS_SET(flags, Type::Hidden); }
};
