#include "Pch.h"
#include "Base.h"
#include "Run.h"
#include "Parse.h"
#include "Function.h"
#include "Cas.h"
#include "Type.h"

Logger* logger;
vector<Type*> types;
vector<Function*> functions;
cas::EventHandler handler;
Tokenizer t;

void InitCoreLib();

bool cas::ParseAndRun(cstring input, bool optimize, bool decompile)
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

	// run
	RunCode(rctx);
	return true;
}

bool cas::AddFunction(cstring decl, void* ptr)
{
	assert(decl && ptr);
	Function* f = ParseFuncDecl(decl);
	if(!f)
	{
		handler(Error, Format("Failed to parse function declaration for AddFunction '%s'.", decl));
		return false;
	}
	Function* f2 = Function::FindEqual(*f);
	if(f2)
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

bool cas::AddMethod(cstring type_name, cstring decl, void* ptr)
{
	assert(type_name && decl && ptr);
	Type* type = Type::Find(type_name);
	if(!type)
	{
		handler(Error, Format("Missing type for AddMethod '%s'.", type_name));
		return false;
	}
	Function* f = ParseFuncDecl(decl);
	if(!f)
	{
		handler(Error, Format("Failed to parse function declaration for AddMethod '%s'.", decl));
		return false;
	}
	Function* f2 = type->FindEqualFunction(*f);
	if(f2)
	{
		handler(Error, Format("Method '%s' for type '%s' already exists.", f->GetName(), type->name.c_str()));
		delete f;
		return false;
	}
	f->clbk = ptr;
	f->index = functions.size();
	f->type = type->index;
	f->arg_infos.insert(f->arg_infos.begin(), ArgInfo(f->type, 0, false));
	f->required_args++;
	type->funcs.push_back(f);
	functions.push_back(f);
	return true;
}

bool cas::AddType(cstring type_name, int size, bool pod)
{
	assert(type_name && size > 0);
	t.CheckItemOrKeyword(type_name);
	if(t.IsKeyword())
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
	type->index = types.size();
	types.push_back(type);
	AddParserType(type);
	return true;
}

bool cas::AddMember(cstring type_name, cstring decl, int offset)
{
	assert(type_name && decl && offset >= 0);
	Type* type = Type::Find(type_name);
	if(!type)
	{
		handler(Error, Format("Missing type for AddMember '%s'.", type_name));
		return false;
	}
	Member* m = ParseMember(decl);
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

static void EmptyEventHandler(cas::EventType, cstring) {}

void cas::SetHandler(EventHandler _handler)
{
	if(_handler)
		handler = _handler;
	else
		handler = &EmptyEventHandler;
}

Function* Type::FindFunction(const string& name)
{
	for(Function* f : funcs)
	{
		if(f->name == name)
			return f;
	}
	return nullptr;
}

Function* Type::FindEqualFunction(Function& fc)
{
	for(Function* f : funcs)
	{
		if(f->name == fc.name && f->Equal(fc))
			return f;
	}
	return nullptr;
}

Member* Type::FindMember(const string& name, int& index)
{
	index = 0;
	for(Member* m : members)
	{
		if(m->name == name)
			return m;
		++index;
	}
	return nullptr;
}

Type* Type::Find(cstring name)
{
	assert(name);
	for(Type* type : types)
	{
		if(type->name == name)
			return type;
	}
	return nullptr;
}

void cas::Initialize()
{
	static bool init = false;
	if(init)
		return;
	InitializeParser();
	InitCoreLib();
	init = true;
}

struct StaticInitializer
{
	StaticInitializer()
	{
		handler = &EmptyEventHandler;
	}
} static_initializer;

cstring CommonFunction::GetName(uint var_offset) const
{
	LocalString s = Format("%s %s(", types[result]->name.c_str(), name.c_str());
	for(uint i = var_offset, count = arg_infos.size(); i < count; ++i)
	{
		if(i != var_offset)
			s += ",";
		s += types[arg_infos[i].type]->name;
	}
	s += ")";
	return Format("%s", s->c_str());
}

bool CommonFunction::Equal(CommonFunction& f) const
{
	if(f.arg_infos.size() != arg_infos.size())
		return false;
	for(uint i = 0, count = arg_infos.size(); i < count; ++i)
	{
		if(arg_infos[i].type != f.arg_infos[i].type)
			return false;
	}
	return true;
}

Function* Function::Find(const string& name)
{
	for(Function* f : functions)
	{
		if(f->name == name && f->type == V_VOID)
			return f;
	}
	return nullptr;
}

Function* Function::FindEqual(Function& fc)
{
	for(Function* f : functions)
	{
		if(f->name == fc.name && f->type == V_VOID && f->Equal(fc))
			return f;
	}
	return nullptr;
}
