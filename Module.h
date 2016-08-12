#pragma once

#include "IModule.h"
#include "Function.h"

class Parser;

class Module : public IModule
{
public:
	Module(int index, Module* parent_module);
	~Module();
	void RemoveRef(bool release);

	// from IModule
	bool AddFunction(cstring decl, void* ptr) override;
	bool AddMethod(cstring type_name, cstring decl, void* ptr) override;
	bool AddType(cstring type_name, int size, bool pod) override;
	bool AddMember(cstring type_name, cstring decl, int offset) override;
	ReturnValue GetReturnValue() override;
	bool ParseAndRun(cstring input, bool optimize = true, bool decompile = false) override;

	template<typename T>
	inline bool AddType(cstring type_name)
	{
		return IModule::AddType<T>(type_name);
	}

	void AddCoreType(cstring type_name, int size, CoreVarType var_type, bool is_ref, bool hidden = false);
	Function* FindEqualFunction(Function& fc);
	Type* FindType(cstring type_name);
	void AddParentModule(Module* parent_module);

	static vector<Module*> all_modules;
	std::map<int, Module*> modules;
	vector<Function*> functions;
	vector<Type*> types;
	ReturnValue return_value;
	Parser* parser;
	int index, refs;
	bool inherited, released;
};
