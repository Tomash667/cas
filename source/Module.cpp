#include "Pch.h"
#include "CasImpl.h"
#include "Module.h"
#include "Function.h"
#include "Parser.h"

vector<Module*> Module::all_modules;
bool Module::all_modules_shutdown;
cas::ReturnValue return_value;

Module::Module(int index, Module* parent_module) : inherited(false), parser(nullptr), index(index), refs(1), released(false)
{
	modules[index] = this;
	if(parent_module)
		AddParentModule(parent_module);

	parser = new Parser(this);

	all_modules.push_back(this);
}

Module::~Module()
{
	DeleteElements(types);
	DeleteElements(functions);
	delete parser;

	if(!all_modules_shutdown)
		RemoveElement(all_modules, this);
}

void Module::RemoveRef(bool release)
{
	if(release)
	{
		assert(!released);
		released = true;
	}

	--refs;
	if(refs == 0)
	{
		for(auto& m : modules)
		{
			if(m.first != index)
				m.second->RemoveRef(false);
		}
		delete this;
	}
}

bool Module::AddFunction(cstring decl, const FunctionInfo& func_info)
{
	assert(decl);
	if(func_info.thiscall)
	{
		Event(EventType::Error, Format("Can't use thiscall in function '%s'.", decl));
		return false;
	}
	Function* f = parser->ParseFuncDecl(decl, nullptr);
	if(!f)
	{
		Event(EventType::Error, Format("Failed to parse function declaration for AddFunction '%s'.", decl));
		return false;
	}
	f->type = V_VOID;
	if(FindEqualFunction(*f))
	{
		Event(EventType::Error, Format("Function '%s' already exists.", parser->GetName(f)));
		delete f;
		return false;
	}
	f->clbk = func_info.ptr;
	if(func_info.builtin)
		f->flags |= CommonFunction::F_BUILTIN;
	f->index = (index << 16) | functions.size();
	functions.push_back(f);
	return true;
}

bool Module::AddMethod(cstring type_name, cstring decl, const FunctionInfo& func_info)
{
	assert(type_name && decl);
	Type* type = FindType(type_name);
	if(!type)
	{
		Event(EventType::Error, Format("Missing type '%s' for AddMethod '%s'.", type_name, decl));
		return false;
	}
	Function* f = parser->ParseFuncDecl(decl, type);
	if(!f)
	{
		Event(EventType::Error, Format("Failed to parse function declaration for AddMethod '%s'.", decl));
		return false;
	}
	f->type = type->index;
	if(f->special == SF_CTOR)
		type->flags |= Type::HaveCtor;
	if(parser->FindEqualFunction(type, AnyFunction(f)))
	{
		Event(EventType::Error, Format("%s '%s' for type '%s' already exists.", f->special <= SF_CTOR ? "Method" : "Special method",
			parser->GetName(f, true, false), type->name.c_str()));
		delete f;
		return false;
	}
	f->clbk = func_info.ptr;
	if(func_info.thiscall)
		f->flags |= CommonFunction::F_THISCALL;
	if(func_info.builtin)
		f->flags |= CommonFunction::F_BUILTIN;
	f->index = (index << 16) | functions.size();
	type->funcs.push_back(f);
	functions.push_back(f);
	return true;
}

bool Module::AddType(cstring type_name, int size, int flags)
{
	assert(type_name && size > 0);
	assert(!inherited); // can't add types to inherited module (until fixed)
	if(IS_SET(flags, DisallowCreate))
		flags |= NoRefCount;
	int type_index;
	if(!parser->VerifyTypeName(type_name, type_index))
	{
		if(type_index == -1)
			Event(EventType::Error, Format("Can't declare type '%s', name is keyword.", type_name));
		else
			Event(EventType::Error, Format("Type '%s' already declared.", type_name));
		return false;
	}
	Type* type = new Type;
	type->name = type_name;
	type->size = size;
	type->flags = flags | Type::Class | Type::Ref;
	type->index = types.size() | (index << 16);
	type->declared = true;
	types.push_back(type);
	parser->AddType(type);
	return true;
}

bool Module::AddMember(cstring type_name, cstring decl, int offset)
{
	assert(type_name && decl && offset >= 0);
	Type* type = FindType(type_name);
	if(!type)
	{
		Event(EventType::Error, Format("Missing type '%s' for AddMember '%s'.", type_name, decl));
		return false;
	}
	Member* m = parser->ParseMemberDecl(decl);
	if(!m)
	{
		Event(EventType::Error, Format("Failed to parse member declaration for type '%s' AddMember '%s'.", type_name, decl));
		return false;
	}
	m->offset = offset;
	int m_index;
	if(type->FindMember(m->name, m_index))
	{
		Event(EventType::Error, Format("Member with name '%s.%s' already exists.", type_name, m->name.c_str()));
		delete m;
		return false;
	}
	assert(offset + parser->GetType(m->vartype.type)->size <= type->size);
	type->members.push_back(m);
	return true;
}

ReturnValue Module::GetReturnValue()
{
	return return_value;
}

bool Module::ParseAndRun(cstring input, bool optimize, bool decompile)
{
	// parse
	ParseSettings settings;
	settings.input = input;
	settings.optimize = optimize;
	RunModule* run_module = parser->Parse(settings);
	if(!run_module)
		return false;

	// decompile
	if(decompile)
		Decompile(*run_module);
		
	// run
	Run(*run_module, return_value);

	// cleanup
	parser->Cleanup();
	return true;
}

void Module::AddCoreType(cstring type_name, int size, CoreVarType var_type, bool is_ref, bool hidden)
{
	// can only be used in core module
	assert(index == 0);
	assert(!inherited);

	Type* type = new Type;
	type->name = type_name;
	type->size = size;
	type->index = types.size();
	assert(type->index == (int)var_type);
	type->flags = 0;
	if(is_ref)
		type->flags |= Type::Ref;
	if(hidden)
		type->flags |= Type::Hidden;
	type->declared = true;
	types.push_back(type);
	if(!hidden)
		parser->AddType(type);
}

Function* Module::FindEqualFunction(Function& fc)
{
	for(auto& module : modules)
	{
		for(Function* f : module.second->functions)
		{
			if(f->name == fc.name && f->type == V_VOID && f->Equal(fc))
				return f;
		}
	}

	return nullptr;
}

Type* Module::FindType(cstring type_name)
{
	assert(type_name);

	for(auto& module : modules)
	{
		for(Type* type : module.second->types)
		{
			if(type->name == type_name)
				return type;
		}
	}

	return nullptr;
}

void Module::AddParentModule(Module* parent_module)
{
	assert(parent_module);

	for(auto& m : parent_module->modules)
	{
		assert(m.first != index); // circular dependency!
		modules[m.first] = m.second;
		m.second->refs++;
		m.second->inherited = true;
	}
}

bool Module::Verify()
{
	/*int errors = 0;
	for(Type* t : types)
	{
		if(!IS_SET(t->flags, Type::NoRefCount))
		{
			if(!t->FindSpecialFunction(SF_ADDREF))
			{
				ERROR(Format("Type '%s' don't have addref operator.", t->name.c_str()));
				++errors;
			}

			if(!t->FindSpecialFunction(SF_RELEASE))
			{
				ERROR(Format("Type '%s' don't have release operator.", t->name.c_str()));
				++errors;
			}
		}

		if(!IS_SET(t->flags, Type::DisallowCreate))
		{
			if(!t->FindSpecialFunction(SF_CTOR))
			{
				ERROR(Format("Type '%s' don't have constructor.", t->name.c_str()));
				++errors;
			}			
		}
	}
	return errors == 0;*/
	return true;
}
