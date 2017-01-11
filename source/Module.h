#pragma once

#include "cas/IModule.h"
#include "Function.h"

class Parser;

class ScriptType : public IType
{
public:
	inline ScriptType(Module* module, Type* type, bool _is_struct) : module(module), type(type)
	{
		is_struct = _is_struct;
	}

	bool AddMember(cstring decl, int offset) override;
	bool AddMethod(cstring decl, const FunctionInfo& func_info) override;

	Module* module;
	Type* type;
};

class ScriptEnum : public IEnum
{
public:
	inline ScriptEnum(Module* module, Type* type) : module(module), type(type) {}

	bool AddValue(cstring name) override;
	bool AddValue(cstring name, int value) override;
	bool AddValues(std::initializer_list<cstring> const& items) override;
	bool AddValues(std::initializer_list<Item> const& items) override;

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
	IEnum* AddEnum(cstring type_name) override;
	ReturnValue GetReturnValue() override;
	cstring GetException() override;
	ExecutionResult ParseAndRun(cstring input, bool optimize = true, bool decompile = false) override;
	
	Type* AddCoreType(cstring type_name, int size, CoreVarType var_type, int flags);
	bool AddMember(Type* type, cstring decl, int offset);
	bool AddMethod(Type* type, cstring decl, const FunctionInfo& func_info);
	Function* FindEqualFunction(Function& fc);
	Type* FindType(cstring type_name);
	void AddParentModule(Module* parent_module);
	bool BuildModule();
	bool AddEnumValue(Type* type, cstring name, int value);
	bool VerifyTypeName(cstring type_name);

	std::map<int, Module*> modules;
	vector<Function*> functions;
	vector<Type*> types;
	vector<ScriptType*> script_types;
	vector<ScriptEnum*> script_enums;
	ReturnValue return_value;
	Parser* parser;
	string exc;
	int index, refs;
	bool inherited, released, built;
	static vector<Module*> all_modules;
	static bool all_modules_shutdown;
};
