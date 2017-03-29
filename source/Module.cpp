#include "Pch.h"
#include "CallContext.h"
#include "Decompiler.h"
#include "Engine.h"
#include "Enum.h"
#include "Event.h"
#include "Global.h"
#include "Member.h"
#include "Module.h"
#include "Parser.h"
#include "Str.h"

Module::Module(int index, cstring name) : index(index), refs(1), built(false), call_context_counter(0), released(false), globals_offset(0)
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
	CodeFunction* f = parser->ParseFuncDecl(decl, nullptr, func_info.builtin);
	if(!f)
	{
		Error("Failed to parse function declaration for AddFunction '%s'.", decl);
		return false;
	}
	f->type = V_VOID;
	if(FindEqualFunction(f))
	{
		Error("Function '%s' already exists.", f->GetFormattedName());
		delete f;
		return false;
	}
	f->clbk = func_info.ptr;
	f->index = (index << 16) | code_funcs.size();
	f->BuildDecl();
	code_funcs.push_back(f);
	return true;
}

bool Module::AddGlobal(cstring decl, void* ptr)
{
	assert(decl && ptr);

	Global* global = parser->ParseGlobalDecl(decl);
	if(!global)
	{
		Error("Failed to parse global declaration '%s'.", decl);
		return false;
	}

	if(GetGlobal(global->name.c_str(), 0))
	{
		Error("Global '%s' already exists.", global->name.c_str());
		delete global;
		return false;
	}

	global->module_proxy = this;
	global->ptr = ptr;
	global->index = (index << 16) | globals.size();
	globals.push_back(global);
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
		if(m.second == added_module || modules.find(m.first) != modules.end())
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

cas::IFunction* Module::GetFunction(cstring name_or_decl, int flags)
{
	assert(name_or_decl);

	if(IS_SET(flags, cas::ByDecl))
	{
		string name, decl;
		if(!parser->GetFunctionNameDecl(name_or_decl, &name, &decl, nullptr))
		{
			Error("Failed to parse function declaration '%s' for GetFunction.", name_or_decl);
			return nullptr;
		}
		return GetFunctionInternal(name.c_str(), decl.c_str(), flags);
	}
	else
		return GetFunctionInternal(name_or_decl, nullptr, flags);
}

void Module::GetFunctionsList(vector<cas::IFunction*>& funcs, cstring name, int flags)
{
	assert(name);

	for(CodeFunction* cf : code_funcs)
	{
		if(cf->type == V_VOID && cf->name == name)
			funcs.push_back(cf);
	}

	for(ScriptFunction* sf : script_funcs)
	{
		if(sf->type == V_VOID && sf->name == name)
			funcs.push_back(sf);
	}

	if(!IS_SET(flags, cas::IgnoreParent))
	{
		for(auto& m : modules)
		{
			if(m.first == index)
				continue;

			for(CodeFunction* cf : m.second->code_funcs)
			{
				if(cf->type == V_VOID && cf->name == name)
					funcs.push_back(cf);
			}

			for(ScriptFunction* sf : m.second->script_funcs)
			{
				if(sf->type == V_VOID && sf->name == name)
					funcs.push_back(sf);
			}
		}
	}
}

cas::IGlobal* Module::GetGlobal(cstring name, int flags)
{
	assert(name);

	for(Global* g : globals)
	{
		if(g->name == name)
			return g;
	}

	if(!IS_SET(flags, cas::IgnoreParent))
	{
		for(auto& m : modules)
		{
			if(m.first == index)
				continue;

			for(Global* g : m.second->globals)
			{
				if(g->name == name)
					return g;
			}
		}
	}

	return nullptr;
}

cstring Module::GetName()
{
	return name.c_str();
}

const cas::IModule::Options& Module::GetOptions()
{
	return options;
}

cas::IType* Module::GetType(cstring name, int flags)
{
	assert(name);

	for(Type* type : types)
	{
		if(type->name == name && !type->IsHidden())
			return type;
	}

	if(!IS_SET(flags, cas::IgnoreParent))
	{
		for(auto& m : modules)
		{
			if(m.first == index)
				continue;
			for(Type* type : m.second->types)
			{
				if(type->name == name && !type->IsHidden())
					return type;
			}
		}
	}

	return nullptr;
}

cas::IModule::ParseResult Module::Parse(cstring input)
{
	assert(input);

	// build
	if(!BuildModule())
		return ValidationError;

	// parse
	ParseSettings settings;
	settings.input = input;
	settings.optimize = options.optimize;
	settings.disallow_global_code = options.disallow_global_code;
	settings.disallow_globals = options.disallow_globals;
	if(!parser->Parse(settings))
		return ParsingError;

	return Ok;
}

cas::IModule::ParseResult Module::ParseAndRun(cstring input)
{
	auto result = Parse(input);
	if(result != Ok)
		return result;

	bool ok = Run();
	return ok ? Ok : RunError;
}

void Module::QueryFunctions(delegate<bool(cas::IFunction*)> pred)
{
	for(CodeFunction* cf : code_funcs)
	{
		if(!pred(cf))
			return;
	}

	for(ScriptFunction* sf : script_funcs)
	{
		if(!pred(sf))
			return;
	}
}

void Module::QueryGlobals(delegate<bool(cas::IGlobal*)> pred)
{
	for(Global* g : globals)
	{
		if(!pred(g))
			return;
	}
}

void Module::QueryTypes(delegate<bool(cas::IType*)> pred)
{
	for(Type* t : types)
	{
		if(!pred(t))
			return;
	}
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

bool Module::Run()
{
	cas::ICallContext* call_context = CreateCallContext(nullptr);
	bool ok = call_context->Run();
	call_context->Release();
	return ok;
}

void Module::SetName(cstring new_name)
{
	if(new_name)
		name = new_name;
	else
		name = Format("Module%d", index);
}

void Module::SetOptions(const Options& new_options)
{
	options = new_options;
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
	m->type = type;
	m->offset = offset;
	m->have_def_value = false;
	int m_index;
	if(type->FindMember(m->name, m_index))
	{
		Error("Member with name '%s.%s' already exists.", type->name.c_str(), m->name.c_str());
		delete m;
		return false;
	}
	assert(offset + GetType(m->vartype.type)->size <= type->size);
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
	CodeFunction* f = parser->ParseFuncDecl(decl, type, func_info.builtin);
	if(!f)
	{
		Error("Failed to parse function declaration for AddMethod '%s'.", decl);
		return false;
	}
	f->type = type->index;
	if(type->FindEqualFunction(f))
	{
		Error("%s '%s' for type '%s' already exists.", f->special <= SF_CTOR ? "Method" : "Special method",
			f->GetFormattedName(true, false), type->name.c_str());
		delete f;
		return false;
	}
	f->clbk = func_info.ptr;
	if(func_info.thiscall)
		f->flags |= Function::F_THISCALL;
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
	f->index = (index << 16) | code_funcs.size();
	f->BuildDecl();
	type->funcs.push_back(f);
	code_funcs.push_back(f);
	return true;
}

bool Module::GetFunctionDecl(cstring decl, string& real_decl, Type* type)
{
	return parser->GetFunctionNameDecl(decl, nullptr, &real_decl, type);
}

cstring Module::GetName(VarType vartype)
{
	Type* t = GetType(vartype.type == V_REF ? vartype.subtype : vartype.type);
	if(vartype.type != V_REF)
		return t->name.c_str();
	else
		return Format("%s&", t->name.c_str());
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

cas::Type Module::VarTypeToType(VarType vartype)
{
	switch(vartype.type)
	{
	case V_VOID:
		return cas::Type(cas::GenericType::Void);
	case V_BOOL:
		return cas::Type(cas::GenericType::Bool);
	case V_CHAR:
		return cas::Type(cas::GenericType::Char);
	case V_INT:
		return cas::Type(cas::GenericType::Int);
	case V_FLOAT:
		return cas::Type(cas::GenericType::Float);
	case V_STRING:
		return cas::Type(cas::GenericType::String);
	case V_REF:
		{
			cas::Type type = VarTypeToType(VarType(vartype.subtype, 0));
			type.is_ref = true;
			return type;
		}
	default:
		{
			Type* t = GetType(vartype.type);
			if(t->IsEnum())
				return cas::Type(cas::GenericType::Enum, t);
			else if(t->IsClass())
				return cas::Type(t->IsStruct() ? cas::GenericType::Struct : cas::GenericType::Class, t);
			else
			{
				assert(0);
				return cas::Type(cas::GenericType::Invalid);
			}
		}
	}
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

AnyFunction Module::FindEqualFunction(Function* f)
{
	for(auto& module : modules)
	{
		for(CodeFunction* cf : module.second->code_funcs)
		{
			if(cf->name == f->name && cf->type == V_VOID && cf->Equal(*f))
				return cf;
		}

		for(ScriptFunction* sf : module.second->script_funcs)
		{
			if(sf->name == f->name && sf->type == V_VOID && sf->Equal(*f))
				return sf;
		}
	}

	return nullptr;
}

AnyFunction Module::FindFunction(const string& name)
{
	for(auto& m : modules)
	{
		for(CodeFunction* cf : m.second->code_funcs)
		{
			if(cf->name == name && cf->type == V_VOID)
				return cf;
		}

		for(ScriptFunction* sf : m.second->script_funcs)
		{
			if(sf->name == name && sf->type == V_VOID)
				return sf;
		}
	}

	return nullptr;
}

Global* Module::FindGlobal(const string& name)
{
	for(auto& m : modules)
	{
		for(Global* g : m.second->globals)
		{
			if(g->name == name)
				return g;
		}
	}

	return nullptr;
}

CodeFunction* Module::GetCodeFunction(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int func_index = (index & 0xFFFF);
	assert(modules.find(module_index) != modules.end());
	Module* m = modules[module_index];
	assert(func_index < (int)m->code_funcs.size());
	return m->code_funcs[func_index];
}

Global* Module::GetGlobal(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int global_index = (index & 0xFFFF);
	assert(modules.find(module_index) != modules.end());
	Module* m = modules[module_index];
	assert(global_index < (int)m->globals.size());
	return m->globals[global_index];
}

ScriptFunction* Module::GetScriptFunction(int index, bool local)
{
	if(local)
	{
		assert(index < (int)script_funcs.size());
		return script_funcs[index];
	}
	else
	{
		int module_index = (index & 0xFFFF0000) >> 16;
		int func_index = (index & 0xFFFF);
		assert(modules.find(module_index) != modules.end());
		Module* m = modules[module_index];
		assert(func_index < (int)m->script_funcs.size());
		return m->script_funcs[func_index];
	}
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

bool Module::IsAttached(IModuleProxy* module_proxy)
{
	assert(module_proxy);
	for(auto& m : modules)
	{
		if(m.second == module_proxy)
			return true;
	}
	return false;
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
	for(auto context : call_contexts)
		context->Reset();

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
		code.clear();
	}

	DeleteElements(code_funcs);
	DeleteElements(script_funcs);
	DeleteElements(types);
	DeleteElements(globals);
}

cas::IFunction* Module::GetFunctionInternal(cstring name, cstring decl, int flags)
{
	for(CodeFunction* cf : code_funcs)
	{
		if(cf->type == V_VOID && cf->name == name && (!decl || cf->decl == decl))
			return cf;
	}

	for(ScriptFunction* sf : script_funcs)
	{
		if(sf->type == V_VOID && sf->name == name && (!decl || sf->decl == decl))
			return sf;
	}

	if(!IS_SET(flags, cas::IgnoreParent))
	{
		for(auto& m : modules)
		{
			if(m.first == index)
				continue;

			for(CodeFunction* cf : m.second->code_funcs)
			{
				if(cf->type == V_VOID && cf->name == name && (!decl || cf->decl == decl))
					return cf;
			}

			for(ScriptFunction* sf : m.second->script_funcs)
			{
				if(sf->type == V_VOID && sf->name == name && (!decl || sf->decl == decl))
					return sf;
			}
		}
	}

	return nullptr;
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
