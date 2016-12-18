#pragma once

#include "cas/IModule.h"
#include "Function.h"

class Parser;

class ScriptType : public IType
{
public:
	bool AddMember(cstring decl, int offset) override;
	bool AddMethod(cstring decl, const FunctionInfo& func_info) override;

	Module* module;
	Type* type;
};

class Module : public IModule
{
public:
	Module(int index, Module* parent_module);
	~Module();
	void RemoveRef(bool release);

	// from IModule
	bool AddFunction(cstring decl, const FunctionInfo& func_info) override;
	IType* AddType(cstring type_name, int size, int flags) override;
	ReturnValue GetReturnValue() override;
	cstring GetException() override;
	ExecutionResult ParseAndRun(cstring input, bool optimize = true, bool decompile = false) override;

	/*template<typename T>
	inline bool AddType(cstring type_name)
	{
		return IModule::AddType<T>(type_name);
	}*/

	Type* AddCoreType(cstring type_name, int size, CoreVarType var_type, int flags);
	bool AddMember(Type* type, cstring decl, int offset);
	bool AddMethod(Type* type, cstring decl, const FunctionInfo& func_info);
	Function* FindEqualFunction(Function& fc);
	Type* FindType(cstring type_name);
	void AddParentModule(Module* parent_module);
	bool BuildModule();

	std::map<int, Module*> modules;
	vector<Function*> functions;
	vector<Type*> types;
	vector<ScriptType*> script_types;
	ReturnValue return_value;
	Parser* parser;
	string exc;
	int index, refs;
	bool inherited, released, built;
	static vector<Module*> all_modules;
	static bool all_modules_shutdown;
};
