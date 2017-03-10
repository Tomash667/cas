#include "Pch.h"
#include "Function.h"
#include "IModuleProxy.h"

uint Function::GetArgCount()
{
	uint args_count = args.size();
	if(PassThis())
		--args_count;
	return args_count;
}

cas::ComplexType Function::GetArgType(uint index)
{
	assert(index < GetArgCount());
	if(PassThis())
		++index;
	return module_proxy->GetComplexType(args[index].GetDeclaredVarType());
}

cas::Value Function::GetArgDefaultValue(uint index)
{
	assert(index < GetArgCount());
	if(PassThis())
		++index;
	Arg& arg = args[index];
	int arg_type;
	cas::Value value;
	if(arg.have_def_value)
		arg_type = V_VOID;
	else
	{
		arg_type = arg.vartype.type;
		value.int_value = arg.value;
	}
	value.type = module_proxy->GetType(arg_type);
	return value;
}

cas::IType* Function::GetClass()
{
	if(type == V_VOID)
		return nullptr;
	else
		return module_proxy->GetType(type);
}

cstring Function::GetDecl()
{
	return decl.c_str();
}

int Function::GetFlags()
{
	return flags;
}

cstring Function::GetName()
{
	return name.c_str();
}

cas::ComplexType Function::GetReturnType()
{
	return module_proxy->GetComplexType(result);
}

bool Function::Equal(Function& f) const
{
	if(f.args.size() != args.size())
		return false;
	for(uint i = 0, count = args.size(); i < count; ++i)
	{
		if(args[i].GetDeclaredVarType() != f.args[i].GetDeclaredVarType())
			return false;
	}
	return true;
}
