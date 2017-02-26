#include "Pch.h"
#include "CallContext.h"
#include "Decompiler.h"
#include "Enum.h"
#include "Event.h"
#include "Member.h"
#include "Module.h"
#include "Parser.h"

/*Module::Module() : released(false)
{
}*/

Module::Module(int index, cstring name) : index(index), parser(nullptr), refs(1), built(false)
{
	modules[index] = this;
	SetName(name);

	Options default_options;
	SetOptions(default_options);
}

Module::~Module()
{
	Cleanup(true);
}

cas::IEnum* Module::AddEnum(cstring type_name)
{
	assert(type_name);

	if(!VerifyTypeName(type_name))
		return nullptr;

	Type* type = new Type;
	type->module_proxy = this;
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
	AddParserType(type);

	built = false;
	return type;
}

bool Module::AddFunction(cstring decl, const cas::FunctionInfo& func_info)
{
	Info("test", 1, 2, 3);
	Info("test2");

	assert(decl);
	if(func_info.thiscall)
	{
		Error("Can't use thiscall in function '%s'.", decl);
		return false;
	}
	Function* f = parser->ParseFuncDecl(decl, nullptr, func_info.builtin);
	if(!f)
	{
		Error("Failed to parse function declaration for AddFunction '%s'.", decl);
		return false;
	}
	f->type = V_VOID;
	if(FindEqualFunction(*f))
	{
		Error("Function '%s' already exists.", parser->GetName(f));
		delete f;
		return false;
	}
	f->clbk = func_info.ptr;
	f->index = (index << 16) | functions.size();
#ifdef _DEBUG
	f->decl = parser->GetName(f);
#endif
	functions.push_back(f);
	return true;
}

bool Module::AddParentModule(cas::IModule* _module)
{
	Module* module = (Module*)_module;

	// is already added?
	if(modules.find(module->index) != modules.end())
		return true;

	// detect circular dependency

	// add
	module->child_modules.push_back(this);
	modules[module->index] = module;
	for(auto m : module->modules)
	{
		modules[m.first] = m.second;
		if(parser)
		{
			//f
		}
	}

	return true;

	/*void Module::AddParentModule(Module* parent_module)
{
	assert(parent_module);

	for(auto& m : parent_module->modules)
	{
		assert(m.first != index); // circular dependency!
		modules[m.first] = m.second;
		m.second->refs++;
		//m.second->inherited = true;
	}
}*/
}

cas::IClass* Module::AddType(cstring type_name, int size, int flags)
{
	assert(type_name && size > 0);
	assert(VerifyFlags(flags));

	if(!VerifyTypeName(type_name))
		return nullptr;

	Type* type = new Type;
	type->module_proxy = this;
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
	AddParserType(type);

	built = false;
	return type;
}

cas::ICallContext* Module::CreateCallContext(cstring name)
{
	return nullptr;
}

void Module::Decompile()
{
	Decompiler::Get().Decompile(*this);
}

cstring Module::GetName()
{
	return name.c_str();
}

cas::IModule::ParseResult Module::Parse(cstring input)
{
	// build
	if(!BuildModule())
		return ParseResult::ValidationError;

	// parse
	ParseSettings settings;
	settings.input = input;
	settings.optimize = optimize;
	if(!parser->Parse(settings))
		return ParseResult::ParsingError;

	return ParseResult::Ok;
}

bool Module::Release()
{
	if(!child_modules.empty())
	{
		Warn("Can't release module that have child modules.");
		return false;
	}

	if(refs > 1)
	{
		Warn("Can't release module that have attached call context.");
		return false;
	}

	for(auto& m : modules)
	{
		if(m.first != index)
		{
			RemoveElement(m.second->child_modules, this);
			--m.second->refs;
		}
	}

	delete this;

	return true;
}

bool Module::Reset()
{
	if(!child_modules.empty())
	{
		Warn("Can't reset module that have child modules.");
		return false;
	}

	if(refs > 1)
	{
		Warn("Can't reset module that have attached call context.");
		return false;
	}

	//Cleanup(false);
	return true;
}

void Module::SetName(cstring new_name)
{
	if(new_name)
		name = new_name;
	else
		name = Format("Module%d", index);
}

void Module::SetOptions(const Options& options)
{
	optimize = options.optimize;
}

bool Module::AddEnumValue(Type* type, cstring name, int value)
{
	int type_index;
	if(!parser->VerifyTypeName(name, type_index))
	{
		Error("Enumerator name '%s' already used as %s.", name, type_index == -1 ? "keyword" : "type");
		return false;
	}

	if(type->enu->Find(name))
	{
		Error("Enumerator '%s.%s' already defined.", type->name.c_str(), name);
		return false;
	}

	type->enu->values.push_back(std::pair<string, int>(name, value));
	return true;
}

bool Module::AddMember(Type* type, cstring decl, int offset)
{
	assert(type && decl && offset >= 0);
	assert(!type->built);
	Member* m = parser->ParseMemberDecl(decl);
	if(!m)
	{
		Error("Failed to parse member declaration for type '%s' AddMember '%s'.", type->name.c_str(), decl);
		return false;
	}
	m->offset = offset;
	m->have_def_value = false;
	int m_index;
	if(type->FindMember(m->name, m_index))
	{
		Error("Member with name '%s.%s' already exists.", type->name.c_str(), m->name.c_str());
		delete m;
		return false;
	}
	assert(offset + parser->GetType(m->vartype.type)->size <= type->size);
	m->index = type->members.size();
	if(m->vartype.type == V_STRING)
		type->have_complex_member = true;
	type->members.push_back(m);
	return true;
}

bool Module::AddMethod(Type* type, cstring decl, const cas::FunctionInfo& func_info)
{
	assert(type && decl);
	assert(!type->built);
	Function* f = parser->ParseFuncDecl(decl, type, func_info.builtin);
	if(!f)
	{
		Error("Failed to parse function declaration for AddMethod '%s'.", decl);
		return false;
	}
	f->type = type->index;
	if(parser->FindEqualFunction(type, AnyFunction(f)))
	{
		Error("%s '%s' for type '%s' already exists.", f->special <= SF_CTOR ? "Method" : "Special method",
			parser->GetName(f, true, false), type->name.c_str());
		delete f;
		return false;
	}
	f->clbk = func_info.ptr;
	if(func_info.thiscall)
		f->flags |= CommonFunction::F_THISCALL;
	if(!func_info.builtin && f->special == SF_CTOR)
	{
		if(type->IsPassByValue())
		{
			if(func_info.return_pointer_or_reference)
			{
				Error("Struct constructor '%s' must return type by value.", decl);
				delete f;
				return false;
			}
		}
		else
		{
			if(!func_info.return_pointer_or_reference)
			{
				Error("Class constructor '%s' must return type by reference/pointer.", decl);
				delete f;
				return false;
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

Type* Module::AddCoreType(cstring type_name, int size, CoreVarType var_type, int flags)
{
	// can only be used in core module
	assert(index == 0);

	Type* type = new Type;
	type->module_proxy = this;
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
		AddParserType(type);
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

Function* Module::GetFunction(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int func_index = (index & 0xFFFF);
	assert(modules.find(module_index) != modules.end());
	Module* m = modules[module_index];
	assert(func_index < (int)m->functions.size());
	return m->functions[func_index];
}

cstring Module::GetFunctionName(uint index, bool is_user)
{
	if(is_user)
		return parser->GetParserFunctionName(index);
	else
		return parser->GetName(parser->GetFunction(index));
}

Str* Module::GetStr(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int str_index = (index & 0xFFFF);
	assert(modules.find(module_index) != modules.end());
	Module* m = modules[module_index];
	assert(str_index < (int)m->strs.size());
	return m->strs[str_index];
}

Type* Module::GetType(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int type_index = (index & 0xFFFF);
	assert(modules.find(module_index) != modules.end());
	Module* m = modules[module_index];
	assert(type_index < (int)m->types.size());
	return m->types[type_index];
}

void Module::AddParserType(Type* type)
{
	if(parser)
		parser->AddType(type);
	for(Module* module : child_modules)
	{
		if(module->parser)
			module->parser->AddType(type);
	}
}

bool Module::BuildModule()
{
	for(auto& m : modules)
	{
		if(m.first == index || m.second->built)
			continue;
		if(!m.second->BuildModule())
			return false;
	}

	if(built)
		return true;

	for(Type* type : types)
	{
		if(!type->IsCode() || type->built)
			continue;

		// verify type
		if(type->IsRefCounted())
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

		if(!parser->CheckTypeLoop(type))
		{
			ERROR(Format("Type '%s' have members loop.", type->name.c_str()));
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

void Module::Cleanup(bool dtor)
{
	/*CleanupReturnValue();

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

	DeleteElements(tmp_types);*/
}

/*void Module::RemoveRef(bool release)
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
}*/

bool Module::VerifyFlags(int flags) const
{
	if(IS_SET(flags, cas::ValueType))
	{
		if(IS_SET(flags, cas::RefCount))
			return false; // struct can't have reference counting
	}
	return true;
}

bool Module::VerifyTypeName(cstring type_name) const
{
	int type_index;
	if(!parser->VerifyTypeName(type_name, type_index))
	{
		if(type_index == -1)
			Error("Can't declare type '%s', name is keyword.", type_name);
		else
			Error("Type '%s' already declared.", type_name);
		return false;
	}
	else
		return true;
}
