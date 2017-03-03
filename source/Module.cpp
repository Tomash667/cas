#include "Pch.h"
#include "CallContext.h"
#include "Decompiler.h"
#include "Engine.h"
#include "Enum.h"
#include "Event.h"
#include "Member.h"
#include "Module.h"
#include "Parser.h"
#include "Str.h"

Module::Module(int index, cstring name) : index(index), refs(1), built(false), call_context_counter(0), released(false)
{
	modules[index] = this;
	SetName(name);

	Options default_options;
	SetOptions(default_options);

	parser = new Parser(*this);

	// initialize empty string
	if(index == 0)
	{
		Str* str = Str::Get();
		str->s = "";
		str->refs = 1;
		strs.push_back(str);
	}
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
	Module* added_module = (Module*)_module;

	// is already added?
	if(modules.find(added_module->index) != modules.end())
		return true;

	// detect circular dependency
	if(added_module->modules.find(index) != added_module->modules.end())
	{
		Error("Can't add parent module '%s' to module '%s', circular dependency.", added_module->name.c_str(), name.c_str());
		return false;
	}

	// add
	added_module->refs++;
	added_module->child_modules.push_back(this);
	modules[added_module->index] = added_module;
	for(Type* type : added_module->types)
	{
		if(!type->IsHidden())
			parser->AddType(type);
	}

	// apply added module parent modules to this module
	for(auto m : added_module->modules)
	{
		if(m.second == added_module || modules.find(m.first) == modules.end())
			continue;
		m.second->refs++;
		m.second->child_modules.push_back(this);
		modules[m.first] = m.second;
		for(Type* type : m.second->types)
		{
			if(!type->IsHidden())
				parser->AddType(type);
		}
	}

	// apply to childs
	for(Module* m : child_modules)
	{
		if(!m->AddParentModule(added_module))
			return false;
	}

	return true;
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
	CallContext* context = new CallContext(call_context_counter++, *this, name);
	call_contexts.push_back(context);
	return context;
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

void Module::Release()
{
	assert(!released);
	released = true;
	RemoveRef();
}

void Module::Reset()
{
	Cleanup(false);
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

void Module::RemoveCallContext(CallContext* call_context)
{
	assert(call_context);
	RemoveElement(call_contexts, call_context);
}

void Module::AddParserType(Type* type)
{
	parser->AddType(type);
	for(Module* module : child_modules)
		module->parser->AddType(type);
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
	if(!dtor)
	{
		for(Module* m : child_modules)
			m->Cleanup(false);
	}

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
		delete parser;
		DeleteElements(call_contexts);
	}
	else
	{
		parser->RemoveKeywords(this);
		for(Module* m : child_modules)
			m->parser->RemoveKeywords(this);
		parser->Reset();
		ufuncs.clear();
		code.clear();
	}

	DeleteElements(functions);
	DeleteElements(types);
}

void Module::RemoveRef()
{
	if(--refs == 0)
	{
		for(auto& m : modules)
		{
			if(m.first != index)
			{
				RemoveElement(m.second->child_modules, this);
				m.second->RemoveRef();
			}
		}

		Engine::Get().RemoveModule(this);
		delete this;
	}
}

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
