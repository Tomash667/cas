#pragma once

#include "cas/IGlobal.h"
#include "VarType.h"

class IModuleProxy;

struct Global : public cas::IGlobal
{
	IModuleProxy* module_proxy;
	string name;
	VarType vartype;

	cas::IModule* GetModule() override;
	cstring GetName() override;
	cas::ComplexType GetType() override;
};
