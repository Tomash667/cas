#include "Pch.h"
#include "Function.h"

cstring Function::GetName()
{
	return name.c_str();
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
