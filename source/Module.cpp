#include "Pch.h"
#include "CasImpl.h"
#include "Module.h"
#include "Function.h"
#include "Parser.h"

vector<Module*> Module::all_modules;
bool Module::all_modules_shutdown;
cas::ReturnValue return_value;

Module::Module(int index, Module* parent_module) : inherited(false), parser(nullptr), index(index), refs(1), released(false), built(false)
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
	DeleteElements(script_types);
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
	else
		f->flags |= CommonFunction::F_CODE;
	f->index = (index << 16) | functions.size();
	functions.push_back(f);
	return true;
}

bool Module::AddMethod(Type* type, cstring decl, const FunctionInfo& func_info)
{
	assert(type && decl);
	assert(!type->built);
	Function* f = parser->ParseFuncDecl(decl, type);
	if(!f)
	{
		Event(EventType::Error, Format("Failed to parse function declaration for AddMethod '%s'.", decl));
		return false;
	}
	f->type = type->index;
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
	else
		f->flags |= CommonFunction::F_CODE;
	f->index = (index << 16) | functions.size();
	type->funcs.push_back(f);
	functions.push_back(f);
	return true;
}

bool VerifyFlags(int flags)
{
	if(IS_SET(flags, ValueType))
	{
		if(IS_SET(flags, RefCount))
			return false; // struct can't have reference counting
	}
	return true;
}

cas::IType* Module::AddType(cstring type_name, int size, int flags)
{
	assert(type_name && size > 0);
	assert(!inherited); // can't add types to inherited module (until fixed)
	assert(VerifyFlags(flags));

	int type_index;
	if(!parser->VerifyTypeName(type_name, type_index))
	{
		if(type_index == -1)
			Event(EventType::Error, Format("Can't declare type '%s', name is keyword.", type_name));
		else
			Event(EventType::Error, Format("Type '%s' already declared.", type_name));
		return nullptr;
	}

	Type* type = new Type;
	type->name = type_name;
	type->size = size;
	type->flags = Type::Class | Type::Code;
	if(IS_SET(flags, cas::ValueType))
		type->flags |= Type::PassByValue;
	else
		type->flags |= Type::Ref;
	if(IS_SET(flags, cas::Complex))
		type->flags |= Type::Complex;
	if(IS_SET(flags, cas::DisallowCreate))
		type->flags |= Type::DisallowCreate;
	if(IS_SET(flags, cas::RefCount))
		type->flags |= Type::RefCount;
	type->index = types.size() | (index << 16);
	type->declared = true;
	type->built = false;
	types.push_back(type);
	parser->AddType(type);

	ScriptType* script_type = new ScriptType;
	script_type->module = this;
	script_type->type = type;
	script_types.push_back(script_type);

	built = false;
	return script_type;
}

bool Module::AddMember(Type* type, cstring decl, int offset)
{
	assert(type && decl && offset >= 0);
	assert(!type->built);
	Member* m = parser->ParseMemberDecl(decl);
	if(!m)
	{
		Event(EventType::Error, Format("Failed to parse member declaration for type '%s' AddMember '%s'.", type->name.c_str(), decl));
		return false;
	}
	m->offset = offset;
	m->have_def_value = false;
	int m_index;
	if(type->FindMember(m->name, m_index))
	{
		Event(EventType::Error, Format("Member with name '%s.%s' already exists.", type->name.c_str(), m->name.c_str()));
		delete m;
		return false;
	}
	assert(offset + parser->GetType(m->vartype.type)->size <= type->size);
	m->index = type->members.size();
	type->members.push_back(m);
	return true;
}

ReturnValue Module::GetReturnValue()
{
	return return_value;
}

cstring Module::GetException()
{
	return exc.c_str();
}

IModule::ExecutionResult Module::ParseAndRun(cstring input, bool optimize, bool decompile)
{
	// build
	if(!BuildModule())
		return ExecutionResult::ValidationError;

	// parse
	ParseSettings settings;
	settings.input = input;
	settings.optimize = optimize;
	RunModule* run_module = parser->Parse(settings);
	if(!run_module)
		return ExecutionResult::ParsingError;

	// decompile
	if(decompile)
		Decompile(*run_module);
		
	// run
	bool ok = Run(*run_module, return_value, exc);

	// cleanup
	parser->Cleanup();
	return (ok ? ExecutionResult::Ok : ExecutionResult::Exception);
}

Type* Module::AddCoreType(cstring type_name, int size, CoreVarType var_type, int flags)
{
	// can only be used in core module
	assert(index == 0);
	assert(!inherited);

	Type* type = new Type;
	type->name = type_name;
	type->size = size;
	type->index = types.size();
	assert(type->index == (int)var_type);
	type->flags = flags;
	type->declared = true;
	type->built = false;
	types.push_back(type);
	if(!IS_SET(flags, Type::Hidden))
		parser->AddType(type);
	built = false;

	return type;
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

bool Module::BuildModule()
{
	if(built)
		return true;

	for(Type* type : types)
	{
		if(!IS_SET(type->flags, Type::Code) || type->built)
			continue;

		// verify type
		if(IS_SET(type->flags, Type::RefCount))
		{
			bool error = false;
			if(!type->FindSpecialCodeFunction(SF_ADDREF))
			{
				ERROR(Format("Type '%s' don't have addref operator.", type->name.c_str()));
				error = true;
			}

			if(!type->FindSpecialCodeFunction(SF_RELEASE))
			{
				ERROR(Format("Type '%s' don't have release operator.", type->name.c_str()));
				error = true;
			}

			if(error)
				return false;
		}

		// create default functions
		int result = parser->CreateDefaultFunctions(type);
		if(IS_SET(result, BF_ASSIGN))
			AddMethod(type, Format("%s operator = (%s obj)", type->name.c_str(), type->name.c_str()), nullptr);
		if(IS_SET(result, BF_EQUAL))
			AddMethod(type, Format("bool operator == (%s obj)", type->name.c_str()), nullptr);
		if(IS_SET(result, BF_NOT_EQUAL))
			AddMethod(type, Format("bool operator != (%s obj)", type->name.c_str()), nullptr);
			
		type->built = true;
	}

	built = true;
	return true;
}

bool ScriptType::AddMember(cstring decl, int offset)
{
	return module->AddMember(type, decl, offset);
}

bool ScriptType::AddMethod(cstring decl, const FunctionInfo& func_info)
{
	return module->AddMethod(type, decl, func_info);
}
