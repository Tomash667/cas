#include "Pch.h"
#include "Base.h"
#include "CasImpl.h"
#include "Module.h"
#include "Function.h"
#include "Parser.h"

vector<Module*> Module::all_modules;

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
	delete parser;

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

bool Module::AddFunction(cstring decl, void* ptr)
{
	assert(decl && ptr);
	Function* f = parser->ParseFuncDecl(decl, nullptr);
	if(!f)
	{
		handler(cas::Error, Format("Failed to parse function declaration for AddFunction '%s'.", decl));
		return false;
	}
	if(FindEqualFunction(*f))
	{
		handler(Error, Format("Function '%s' already exists.", f->GetName()));
		delete f;
		return false;
	}
	f->clbk = ptr;
	f->index = functions.size();
	f->type = V_VOID;
	functions.push_back(f);
	return true;
}

bool Module::AddMethod(cstring type_name, cstring decl, void* ptr)
{
	assert(type_name && decl && ptr);
	Type* type = FindType(type_name);
	if(!type)
	{
		handler(Error, Format("Missing type for AddMethod '%s'.", type_name));
		return false;
	}
	Function* f = parser->ParseFuncDecl(decl, type);
	if(!f)
	{
		handler(Error, Format("Failed to parse function declaration for AddMethod '%s'.", decl));
		return false;
	}
	if(type->FindEqualFunction(*f))
	{
		handler(Error, Format("Method '%s' for type '%s' already exists.", f->GetName(), type->name.c_str()));
		delete f;
		return false;
	}
	f->clbk = ptr;
	f->index = functions.size();
	f->type = type->index;
	if(f->special == SF_CTOR)
		type->have_ctor = true;
	else
	{
		f->arg_infos.insert(f->arg_infos.begin(), ArgInfo(VarType(f->type), 0, false));
		f->required_args++;
	}
	type->funcs.push_back(f);
	functions.push_back(f);
	return true;
}

bool Module::AddType(cstring type_name, int size, bool pod)
{
	assert(type_name && size > 0);
	assert(!inherited); // can't add types to inherited module (until fixed)
	if(!parser->VerifyTypeName(type_name))
	{
		handler(Error, Format("Can't declare type '%s', name is keyword.", type_name));
		return false;
	}
	Type* type = Type::Find(type_name);
	if(type)
	{
		handler(Error, Format("Type '%s' already declared.", type_name));
		return false;
	}
	type = new Type;
	type->name = type_name;
	type->size = size;
	type->pod = pod;
	type->have_ctor = false;
	type->is_ref = true;
	type->index = types.size() + (index << 16);
	types.push_back(type);
	parser->AddType(type);
	return true;
}

bool Module::AddMember(cstring type_name, cstring decl, int offset)
{
	assert(type_name && decl && offset >= 0);
	Type* type = Type::Find(type_name);
	if(!type)
	{
		handler(Error, Format("Missing type for AddMember '%s'.", type_name));
		return false;
	}
	Member* m = parser->ParseMemberDecl(decl);
	if(!m)
	{
		handler(Error, Format("Failed to parse member declaration for AddMemeber '%s'.", decl));
		return false;
	}
	m->offset = offset;
	int m_index;
	if(type->FindMember(m->name, m_index))
	{
		handler(Error, Format("Member with name '%s.%s' already exists.", type_name, m->name.c_str()));
		delete m;
		return false;
	}
	assert(offset + types[m->type]->size <= type->size);
	type->members.push_back(m);
	return true;
}

ReturnValue Module::GetReturnValue()
{
	return return_value;
}

bool Module::ParseAndRun(cstring input, bool optimize, bool decompile)
{
	CleanupParser();

	// parse
	ParseContext ctx;
	ctx.input = input;
	ctx.optimize = optimize;
	if(!Parse(ctx))
		return false;

	// decompile
	if(decompile)
		Decompile(ctx);

	// convert
	RunContext rctx;
	rctx.globals = ctx.globals;
	rctx.entry_point = ctx.entry_point;
	rctx.code = std::move(ctx.code);
	rctx.strs = std::move(ctx.strs);
	rctx.ufuncs = std::move(ctx.ufuncs);
	rctx.result = ctx.result;

	// run
	RunCode(rctx);
	return true;

	return false;
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
	type->pod = true;
	type->have_ctor = false;
	type->is_ref = is_ref;
	type->hidden = hidden;
	types.push_back(type);
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
	assert(name);

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
