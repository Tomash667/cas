#include "Pch.h"
#include "Base.h"
#include "Run.h"
#include "Parse.h"
#include "Function.h"
#include "Cas.h"
#include "Type.h"

VarInfo var_info[V_MAX] = {
	V_VOID, "void", true,
	V_BOOL, "bool", true,
	V_INT, "int", true,
	V_FLOAT, "float", true,
	V_STRING, "string", true
};

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
	Function* f2 = FindFunction(f->name);
	if(f2)
	{
		handler(Error, Format("Function with name '%s' already exists.", f->name.c_str()));
		delete f;
		return false;
	}
	f->clbk = ptr;
	f->index = functions.size();
	f->type = nullptr;
	functions.push_back(f);
	return true;
}

bool cas::AddMethod(cstring type_name, cstring decl, void* ptr)
{
	assert(type_name && decl && ptr);
	Type* type = FindType(type_name);
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
	Function* f2 = type->FindFunction(f->name);
	if(f2)
	{
		handler(Error, Format("Method with name '%s.%s' already exists.", type->name.c_str(), f->name.c_str()));
		delete f;
		return false;
	}
	f->clbk = ptr;
	f->index = functions.size();
	f->type = type;
	f->arg_infos.insert(f->arg_infos.begin(), ArgInfo(f->type->builtin_type));
	f->required_args++;
	type->funcs.push_back(f);
	functions.push_back(f);
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
