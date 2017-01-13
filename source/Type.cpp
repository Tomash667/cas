#include "Pch.h"
#include "Type.h"
#include "Module.h"

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

cstring Type::GetName() const
{
	return name.c_str();
}

bool Type::AddMember(cstring decl, int offset)
{
	return module->AddMember(this, decl, offset);
}

bool Type::AddMethod(cstring decl, const FunctionInfo& func_info)
{
	return module->AddMethod(this, decl, func_info);
}

bool Type::AddValue(cstring name)
{
	assert(name);
	int value;
	if(enu->values.empty())
		value = 0;
	else
		value = enu->values.back().second + 1;
	return module->AddEnumValue(this, name, value);
}

bool Type::AddValue(cstring name, int value)
{
	assert(name);
	return module->AddEnumValue(this, name, value);
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
		if(!module->AddEnumValue(this, name, value))
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
		if(!module->AddEnumValue(this, item.name, item.value))
			return false;
	}
	return true;
}
