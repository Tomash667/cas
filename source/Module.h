#pragma once

#include "cas/IModule.h"
#include "Function.h"

using namespace cas;

class Parser;

class Module : public IModule
{
public:
	Module(int index, Module* parent_module);
	~Module();
	void RemoveRef(bool release);

	// from IModule
	bool AddFunction(cstring decl, const FunctionInfo& func_info) override;
	IClass* AddType(cstring type_name, int size, int flags) override;
	IEnum* AddEnum(cstring type_name) override;
	ReturnValue GetReturnValue() override;
	cstring GetException() override;
	ExecutionResult ParseAndRun(cstring input, bool decompile) override;
	ExecutionResult Parse(cstring input) override;
	ExecutionResult Run() override;
	void SetOptions(const Options& options) override;
	void ResetParser() override;
	void Decompile() override;
	
	Type* AddCoreType(cstring type_name, int size, CoreVarType var_type, int flags);
	bool AddMember(Type* type, cstring decl, int offset);
	bool AddMethod(Type* type, cstring decl, const FunctionInfo& func_info);
	Function* FindEqualFunction(Function& fc);
	Type* FindType(cstring type_name);
	Type* GetType(int index);
	Function* GetFunction(int index);
	void AddParentModule(Module* parent_module);
	bool BuildModule();
	bool AddEnumValue(Type* type, cstring name, int value);
	bool VerifyTypeName(cstring type_name);
	cstring GetFunctionName(uint index, bool is_user);

private:
	void Cleanup(bool dtor);

public:
	std::map<int, Module*> modules;
	vector<Function*> functions;
	vector<UserFunction> ufuncs;
	vector<Type*> types, tmp_types;
	vector<Str*> strs;
	vector<int> code;
	ReturnValue return_value;
	Parser* parser;
	string exc;
	int index, refs;
	uint globals, entry_point;
	bool inherited, released, built, optimize;

	static vector<Module*> all_modules;
	static bool all_modules_shutdown;
};
