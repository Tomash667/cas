#pragma once

#include "cas/IModule.h"
#include "Function.h"

class Parser;

class Module : public IModule
{
public:
	Module(int index, Module* parent_module);
	~Module();
	void RemoveRef(bool release);

	// from IModule
	bool AddFunction(cstring decl, const FunctionInfo& func_info) override;
	bool AddMethod(cstring type_name, cstring decl, const FunctionInfo& func_info) override;
	bool AddType(cstring type_name, int size, int flags) override;
	bool AddMember(cstring type_name, cstring decl, int offset) override;
	ReturnValue GetReturnValue() override;
	bool ParseAndRun(cstring input, bool optimize = true, bool decompile = false) override;
	bool Verify() override;

	template<typename T>
	inline bool AddType(cstring type_name)
	{
		return IModule::AddType<T>(type_name);
	}

	void AddCoreType(cstring type_name, int size, CoreVarType var_type, int flags);
	Function* FindEqualFunction(Function& fc);
	Type* FindType(cstring type_name);
	void AddParentModule(Module* parent_module);
	void BuildModule();

	std::map<int, Module*> modules;
	vector<Function*> functions;
	vector<Type*> types;
	ReturnValue return_value;
	Parser* parser;
	int index, refs;
	bool inherited, released, built;
	static vector<Module*> all_modules;
	static bool all_modules_shutdown;
};
