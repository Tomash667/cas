#pragma once

#include "cas/IGlobal.h"
#include "VarType.h"

class IModuleProxy;

struct Global final : public cas::IGlobal
{
	IModuleProxy* module_proxy;
	string name;
	VarType vartype;
	int index, def_value;
	void* ptr;
	bool have_def_value;

	cas::IModule* GetModule() override;
	cstring GetName() override;
	cas::Type GetType() override;
};
