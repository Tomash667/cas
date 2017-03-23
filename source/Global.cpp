#include "Pch.h"
#include "Global.h"
#include "IModuleProxy.h"

cas::IModule* Global::GetModule()
{
	return module_proxy;
}

cstring Global::GetName()
{
	return name.c_str();
}

cas::Type Global::GetType()
{
	return module_proxy->VarTypeToType(vartype);
}
