#include "Pch.h"
#include "Enum.h"
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

CodeFunction* Type::FindCodeFunction(cstring name)
{
	for(CodeFunction* f : funcs)
	{
		if(f->name == name)
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
	for(CodeFunction* f : funcs)
	{
		if(f->special == special)
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
