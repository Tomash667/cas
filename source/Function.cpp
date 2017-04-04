#include "Pch.h"
#include "Function.h"
#include "IModuleProxy.h"
#include "Symbol.h"

uint Function::GetArgCount()
{
	uint args_count = args.size();
	if(PassThis())
		--args_count;
	return args_count;
}

cas::Type Function::GetArgType(uint index)
{
	assert(index < GetArgCount());
	if(PassThis())
		++index;
	return module_proxy->VarTypeToType(args[index].GetDeclaredVarType());
}

cas::Value Function::GetArgDefaultValue(uint index)
{
	assert(index < GetArgCount());
	if(PassThis())
		++index;

	Arg& arg = args[index];
	cas::Value value;
	if(!arg.have_def_value)
	{
		value.type = cas::Type(cas::GenericType::Void);
		value.int_value = 0;
	}
	else
	{
		value.type = module_proxy->VarTypeToType(arg.vartype);
		value.int_value = arg.value;
		if(value.type.generic_type == cas::GenericType::Class || value.type.generic_type == cas::GenericType::Struct)
			value.type.generic_type = cas::GenericType::Object;
	}

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

cas::IModule* Function::GetModule()
{
	return module_proxy;
}

cstring Function::GetName()
{
	return name.c_str();
}

cas::Type Function::GetReturnType()
{
	return module_proxy->VarTypeToType(result);
}

void Function::BuildDecl()
{
	decl = GetFormattedName(true, false);
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

cstring Function::GetFormattedName(bool write_result, bool write_type, BASIC_SYMBOL* symbol)
{
	LocalString s = "";

	// return type
	if(write_result && special != SF_CTOR && special != SF_DTOR)
	{
		s += module_proxy->GetName(result);
		s += ' ';
	}

	// type
	uint var_offset = 0;
	if(type != V_VOID)
	{
		if(write_type)
		{
			s += module_proxy->GetType(type)->name;
			s += '.';
		}
		if(PassThis())
			++var_offset;
	}

	// name
	if(!symbol)
	{
		if(name[0] == '$')
		{
			if(name == "$opCast")
				s += "operator cast";
			else if(name == "$opAddref")
				s += "operator addref";
			else if(name == "$opRelease")
				s += "operator release";
			else
			{
				for(int i = 0; i < S_MAX; ++i)
				{
					SymbolInfo& si = symbols[i];
					if(si.op_code && strcmp(si.op_code, name.c_str()) == 0)
					{
						s += "operator ";
						s += si.oper;
						s += ' ';
						break;
					}
				}
			}
		}
		else
			s += name;
	}
	else
	{
		s += "operator ";
		s += basic_symbols[*symbol].GetOverloadText();
		s += ' ';
	}

	// args
	s += '(';
	for(uint i = var_offset, count = args.size(); i < count; ++i)
	{
		if(i != var_offset)
			s += ',';
		s += module_proxy->GetName(args[i].vartype);
		if(args[i].pass_by_ref)
			s += '&';
	}
	s += ')';

	return Format("%s", s->c_str());
}
