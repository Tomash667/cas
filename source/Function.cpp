#include "Pch.h"
#include "Function.h"

bool CommonFunction::Equal(CommonFunction& f) const
{
	if(f.arg_infos.size() != arg_infos.size())
		return false;
	for(uint i = 0, count = arg_infos.size(); i < count; ++i)
	{
		if(arg_infos[i].GetDeclaredVarType() != f.arg_infos[i].GetDeclaredVarType())
			return false;
	}
	return true;
}
