#include "Pch.h"
#include "Enum.h"
#include "Event.h"
#include "Function.h"
#include "IModuleProxy.h"
#include "Member.h"
#include "Type.h"

Type::~Type()
{
	delete enu;
	DeleteElements(members);
}

cstring Type::GetName() const
{
	return name.c_str();
}

cas::IFunction* Type::GetFunction(cstring name_or_decl, int flags)
{
	assert(name_or_decl);

	if(IS_SET(flags, cas::ByDecl))
	{
		string decl;
		if(!module_proxy->GetFunctionDecl(name_or_decl, decl, this))
		{
			Error("Failed to parse method declaration '%s' for GetFunction.", name_or_decl);
			return nullptr;
		}

		for(AnyFunction& f : funcs)
		{
			if(f.f->decl == decl)
				return f.f;
		}
	}
	else
	{
		for(AnyFunction& f : funcs)
		{
			if(f.f->name == name_or_decl)
				return f.f;
		}
	}

	return nullptr;
}

void Type::GetFunctionsList(vector<cas::IFunction*>& _funcs, cstring name, int flags)
{
	assert(name);

	for(AnyFunction& f : funcs)
	{
		if(f.f->name == name)
			_funcs.push_back(f.f);
	}
}

bool Type::AddMember(cstring decl, int offset)
{
	return module_proxy->AddMember(this, decl, offset);
}

bool Type::AddMethod(cstring decl, const cas::FunctionInfo& func_info)
{
	return module_proxy->AddMethod(this, decl, func_info);
}

bool Type::AddValue(cstring name)
{
	assert(name);
	int value;
	if(enu->values.empty())
		value = 0;
	else
		value = enu->values.back().second + 1;
	return module_proxy->AddEnumValue(this, name, value);
}

bool Type::AddValue(cstring name, int value)
{
	assert(name);
	return module_proxy->AddEnumValue(this, name, value);
}

bool Type::AddValues(std::initializer_list<cstring> const& items)
{
	int value;
	if(enu->values.empty())
		value = 0;
	else
		value = enu->values.back().second + 1;
	for(cstring name : items)
	{
		assert(name);
		if(!module_proxy->AddEnumValue(this, name, value))
			return false;
		++value;
	}
	return true;
}

bool Type::AddValues(std::initializer_list<Item> const& items)
{
	for(const Item& item : items)
	{
		assert(item.name);
		if(!module_proxy->AddEnumValue(this, item.name, item.value))
			return false;
	}
	return true;
}

void Type::FindAllCtors(vector<AnyFunction>& _funcs)
{
	for(AnyFunction& f : funcs)
	{
		if(f.f->special == SF_CTOR)
			_funcs.push_back(f);
	}
}

void Type::FindAllFunctionOverloads(const string& name, vector<AnyFunction>& _funcs)
{
	for(AnyFunction& f : funcs)
	{
		if(f.f->name == name)
			_funcs.push_back(f);
	}
}

void Type::FindAllStaticFunctionOverloads(const string& name, vector<AnyFunction>& _funcs)
{
	for(AnyFunction& f : funcs)
	{
		if(f.f->IsStatic() && f.f->name == name)
			_funcs.push_back(f);
	}
}

AnyFunction Type::FindEqualFunction(AnyFunction func)
{
	Function& f2 = *func.f;

	if(f2.special == SF_NO || f2.special == SF_CTOR)
	{
		for(AnyFunction& f : funcs)
		{
			if(f.f->name == f2.name && f.f->Equal(f2))
				return f;
		}
	}
	else if(f2.special != SF_CAST)
	{
		for(AnyFunction& f : funcs)
		{
			if(f.f->special == f2.special)
				return f;
		}
	}
	else
	{
		assert(f2.special == SF_CAST);

		for(AnyFunction& f : funcs)
		{
			if(f.f->special == f2.special && f.f->result == f2.result)
				return f;
		}
	}

	return nullptr;
}

AnyFunction Type::FindFunction(const string& name)
{
	for(AnyFunction& f : funcs)
	{
		if(f.f->name == name)
			return f;
	}

	return nullptr;
}

AnyFunction Type::FindFunction(cstring name, delegate<bool(AnyFunction& f)> pred)
{
	assert(name);

	for(AnyFunction& f : funcs)
	{
		if(f.f->name == name && pred(f))
			return f;
	}

	return nullptr;
}

Member* Type::FindMember(const string& name, int& index)
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

CodeFunction* Type::FindSpecialCodeFunction(SpecialFunction special)
{
	for(AnyFunction& f : funcs)
	{
		if(f.IsCode() && f.f->special == special)
			return f.cf;
	}
	return nullptr;
}

AnyFunction Type::FindSpecialFunction(SpecialFunction spec, delegate<bool(AnyFunction& f)> pred)
{
	for(AnyFunction& f : funcs)
	{
		if(f.f->special == spec && pred(f))
			return f;
	}

	return nullptr;
}

void Type::SetGenericType()
{
	switch(index)
	{
	case V_VOID:
		generic_type = GenericType::Void;
		break;
	case V_BOOL:
		generic_type = GenericType::Bool;
		break;
	case V_CHAR:
		generic_type = GenericType::Char;
		break;
	case V_INT:
		generic_type = GenericType::Int;
		break;
	case V_FLOAT:
		generic_type = GenericType::Float;
		break;
	case V_STRING:
		generic_type = GenericType::String;
		break;
	default:
		if(IsEnum())
			generic_type = GenericType::Enum;
		else if(IsStruct())
			generic_type = GenericType::Struct;
		else if(IsRefClass())
			generic_type = GenericType::Class;
		else
			generic_type = GenericType::Invalid;
		break;
	}
}
