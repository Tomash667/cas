#include "Pch.h"
#include "IModuleProxy.h"
#include "Member.h"
#include "Type.h"

cas::IType* Member::GetClass()
{
	return type;
}

cas::IModule* Member::GetModule()
{
	return type->module_proxy;
}

cstring Member::GetName()
{
	return name.c_str();
}

uint Member::GetOffset()
{
	return offset;
}

cas::ComplexType Member::GetType()
{
	return type->module_proxy->GetComplexType(vartype);
}
