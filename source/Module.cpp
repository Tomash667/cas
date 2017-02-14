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
	Options default_options;
	SetOptions(default_options);

	modules[index] = this;
	if(parent_module)
		AddParentModule(parent_module);

	parser = new Parser(this);

	all_modules.push_back(this);
}

Module::~Module()
{
	Cleanup(true);

	if(!all_modules_shutdown)
		RemoveElement(all_modules, this);
}

void Module::Cleanup(bool dtor)
{
	CleanupReturnValue();

	for(Str* s : strs)
	{
		if(s->refs == 1)
			s->Free();
		else
			s->refs--;
	}
	strs.clear();

	if(dtor)
	{
		DeleteElements(types);
		DeleteElements(functions);
		delete parser;
	}
	else
	{
		parser->Reset();
		ufuncs.clear();
		code.clear();
	}

	DeleteElements(tmp_types);
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
#ifdef _DEBUG
	f->decl = parser->GetName(f);
#endif
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
	{
		f->flags |= CommonFunction::F_CODE;
		if(f->special == SF_CTOR)
		{
			if(IS_SET(type->flags, Type::PassByValue))
			{
				if(func_info.return_pointer_or_reference)
				{
					Event(EventType::Error, Format("Struct constructor '%s' must return type by value.", decl));
					delete f;
					return false;
				}
			}
			else
			{
				if(!func_info.return_pointer_or_reference)
				{
					Event(EventType::Error, Format("Class constructor '%s' must return type by reference/pointer.", decl));
					delete f;
					return false;
				}
			}
		}
	}
	f->index = (index << 16) | functions.size();
	type->funcs.push_back(f);
#ifdef _DEBUG
	f->decl = parser->GetName(f);
#endif
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

bool Module::VerifyTypeName(cstring type_name)
{
	int type_index;
	if(!parser->VerifyTypeName(type_name, type_index))
	{
		if(type_index == -1)
			Event(EventType::Error, Format("Can't declare type '%s', name is keyword.", type_name));
		else
			Event(EventType::Error, Format("Type '%s' already declared.", type_name));
		return false;
	}
	else
		return true;
}

cas::IClass* Module::AddType(cstring type_name, int size, int flags)
{
	assert(type_name && size > 0);
	assert(!inherited); // can't add types to inherited module (until fixed)
	assert(VerifyFlags(flags));

	if(!VerifyTypeName(type_name))
		return nullptr;

	Type* type = new Type;
	type->module = this;
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
	type->SetGenericType();
	types.push_back(type);
	parser->AddType(type);

	built = false;
	return type;
}

cas::IEnum* Module::AddEnum(cstring type_name)
{
	assert(type_name);
	assert(!inherited); // can't add types to inherited module (until fixed)

	if(!VerifyTypeName(type_name))
		return nullptr;

	Type* type = new Type;
	type->module = this;
	type->name = type_name;
	type->size = sizeof(int);
	type->flags = Type::Code;
	type->index = types.size() | (index << 16);
	type->declared = true;
	type->built = false;
	type->enu = new Enum;
	type->enu->type = type;
	type->SetGenericType();
	types.push_back(type);
	parser->AddType(type);

	built = false;
	return type;
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

IModule::ExecutionResult Module::ParseAndRun(cstring input, bool decompile)
{
	// build
	if(!BuildModule())
		return ExecutionResult::ValidationError;

	// parse
	ParseSettings settings;
	settings.input = input;
	settings.optimize = optimize;
	if(!parser->Parse(settings))
		return ExecutionResult::ParsingError;

	// decompile
	if(decompile)
		Decompiler::Get().Decompile(*this);
		
	// run
	bool ok = ::Run(*this);

	return (ok ? ExecutionResult::Ok : ExecutionResult::Exception);
}

Type* Module::AddCoreType(cstring type_name, int size, CoreVarType var_type, int flags)
{
	// can only be used in core module
	assert(index == 0);
	assert(!inherited);

	Type* type = new Type;
	type->module = this;
	type->name = type_name;
	type->size = size;
	type->index = types.size();
	assert(type->index == (int)var_type);
	type->flags = flags;
	type->declared = true;
	type->built = false;
	type->SetGenericType();
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

Type* Module::GetType(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int type_index = (index & 0xFFFF);
	if(module_index == 0xFFFF)
	{
		assert(type_index < (int)tmp_types.size());
		return tmp_types[type_index];
	}
	else
	{
		assert(modules.find(module_index) != modules.end());
		Module* m = modules[module_index];
		assert(type_index < (int)m->types.size());
		return m->types[type_index];
	}
}

Function* Module::GetFunction(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int func_index = (index & 0xFFFF);
	assert(modules.find(module_index) != modules.end());
	Module* m = modules[module_index];
	assert(func_index < (int)m->functions.size());
	return m->functions[func_index];
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
			AddMethod(type, Format("%s& operator = (%s& obj)", type->name.c_str(), type->name.c_str()), nullptr);
		if(IS_SET(result, BF_EQUAL))
			AddMethod(type, Format("bool operator == (%s& obj)", type->name.c_str()), nullptr);
		if(IS_SET(result, BF_NOT_EQUAL))
			AddMethod(type, Format("bool operator != (%s& obj)", type->name.c_str()), nullptr);
			
		type->built = true;
	}

	built = true;
	return true;
}

bool Module::AddEnumValue(Type* type, cstring name, int value)
{
	int type_index;
	if(!parser->VerifyTypeName(name, type_index))
	{
		Event(EventType::Error, Format("Enumerator name '%s' already used as %s.", name, type_index == -1 ? "keyword" : "type"));
		return false;
	}

	if(type->enu->Find(name))
	{
		Event(EventType::Error, Format("Enumerator '%s.%s' already defined.", type->name.c_str(), name));
		return false;
	}

	type->enu->values.push_back(std::pair<string, int>(name, value));
	return true;
}

IModule::ExecutionResult Module::Parse(cstring input)
{
	// build
	if(!BuildModule())
		return ExecutionResult::ValidationError;

	// parse
	ParseSettings settings;
	settings.input = input;
	settings.optimize = optimize;
	if(!parser->Parse(settings))
		return ExecutionResult::ParsingError;

	return ExecutionResult::Ok;
}

IModule::ExecutionResult Module::Run()
{
	// run
	bool ok = ::Run(*this);

	return (ok ? ExecutionResult::Ok : ExecutionResult::Exception);
}

void Module::SetOptions(const Options& options)
{
	optimize = options.optimize;
}

void Module::ResetParser()
{
	Cleanup(false);
}

cstring Module::GetFunctionName(uint index, bool is_user)
{
	if(is_user)
		return parser->GetParserFunctionName(index);
	else
		return parser->GetName(parser->GetFunction(index));
}

void Module::Decompile()
{
	Decompiler::Get().Decompile(*this);
}
