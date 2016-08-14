#include "Pch.h"
#include "Base.h"
#include "CasImpl.h"
#include "RunModule.h"
#include "Module.h"

RunModule::RunModule(Module* parent) : parent(parent)
{
	++parent->refs;
}

RunModule::~RunModule()
{
	parent->RemoveRef(false);
}

Type* RunModule::GetType(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int type_index = (index & 0xFFFF);
	if(module_index == 0xFFFF)
	{
		assert(type_index < (int)types.size());
		return types[type_index];
	}
	else
	{
		assert(parent->modules.find(module_index) != parent->modules.end());
		Module* m = parent->modules[module_index];
		assert(type_index < (int)m->types.size());
		return m->types[type_index];
	}
}

Function* RunModule::GetFunction(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int func_index = (index & 0xFFFF);
	assert(parent->modules.find(module_index) != parent->modules.end());
	Module* m = parent->modules[module_index];
	assert(func_index < (int)m->functions.size());
	return m->functions[func_index];
}
