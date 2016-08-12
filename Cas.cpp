#include "Pch.h"
#include "Base.h"
#include "Run.h"
#include "Function.h"
#include "CasImpl.h"
#include "Type.h"
#include "Module.h"

using namespace cas;

Logger* logger;
EventHandler handler;
static Module* core_module;
static int module_index;
static bool initialized;

IModule* CreateModule()
{
	assert(initialized);
	if(!initialized)
		return nullptr;

	Module* module = new Module(module_index, core_module);
	++module_index;

	return module;
}

void DestroyModule(IModule* _module)
{
	Module* module = (Module*)_module;

	assert(initialized);
	assert(module && !module->released);
	if(!initialized || !module || module->released)
		return;

	module->RemoveRef(true);
}

void Initialize(Settings* settings)
{
	assert(!initialized);
	if(initialized)
		return;

	if(!handler)
		SetHandler(nullptr);
	std::istream* input = &cin;
	std::ostream* output = &cout;
	bool use_getch = true;
	if(settings)
	{
		input = (std::istream*)settings->input;
		output = (std::ostream*)settings->output;
		use_getch = settings->use_getch;
	}
	module_index = 1;
	core_module = new Module(0, nullptr);
	InitCoreLib(core_module, input, output, use_getch);

	initialized = true;
}

void Shutdown()
{
	assert(initialized);
	if(!initialized)
		return;

	initialized = false;
	for(Module* m : Module::all_modules)
		delete m;
	Module::all_modules.clear();
}

void SetHandler(EventHandler _handler)
{
	if(_handler)
		handler = _handler;
	else
		handler = [](cas::EventType, cstring) {};
}

//=============================================================================================================================================================
/*Member* Type::FindMember(const string& name, int& index)
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

void AddName(LocalString& s, const VarType& type)
{
	s += types[type.core]->name;
	if(type.special == SV_REF)
		s += '&';
}

cstring CommonFunction::GetName(bool write_result) const
{
	LocalString s = "";
	if(write_result && special != SF_CTOR)
	{
		AddName(s, result);
		s += ' ';
	}
	uint var_offset = 0;
	if(type != V_VOID)
	{
		s += types[type]->name;
		s += '.';
		++var_offset;
	}
	s += name;
	s += '(';
	for(uint i = var_offset, count = arg_infos.size(); i < count; ++i)
	{
		if(i != var_offset)
			s += ",";
		AddName(s, arg_infos[i].type);
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
}*/
