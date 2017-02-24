#pragma once

#include "cas/IModule.h"
#include "Function.h"
#include "IModuleProxy.h"

class CallContext;
class Parser;
struct Str;

// Script module
// Container for code, types and functions
// Can have child and parent modules
class Module : public cas::IModule, public IModuleProxy
{
public:
	Module(int index, cstring name);
	~Module();
	//void RemoveRef(bool release);

	// from IModule
	cas::IEnum* AddEnum(cstring type_name) override;
	bool AddFunction(cstring decl, const cas::FunctionInfo& func_info) override;
	bool AddParentModule(IModule* module) override;
	cas::IClass* AddType(cstring type_name, int size, int flags) override;
	void Decompile() override;
	cstring GetName() override;
	ParseResult Parse(cstring input) override;
	bool Release() override;
	bool Reset() override;
	void SetName(cstring name) override;
	void SetOptions(const Options& options) override;

	// from IModuleProxy
	bool AddEnumValue(Type* type, cstring name, int value);
	bool AddMember(Type* type, cstring decl, int offset);
	bool AddMethod(Type* type, cstring decl, const cas::FunctionInfo& func_info);

	Type* AddCoreType(cstring type_name, int size, CoreVarType var_type, int flags);
	void AddParentModule(Module* parent_module);
	bool BuildModule();
	Function* FindEqualFunction(Function& fc);
	Type* FindType(cstring type_name);
	Type* GetType(int index);
	Function* GetFunction(int index);
	cstring GetFunctionName(uint index, bool is_user);
	Str* GetStr(int index);
	bool VerifyTypeName(cstring type_name);

private:
	void Cleanup(bool dtor);
	void AddParserType(Type* type);

public:
	Parser* parser;
	string name;
	std::map<int, Module*> modules;
	vector<Module*> child_modules;
	vector<Function*> functions;
	vector<UserFunction> ufuncs;
	vector<Type*> types;
	vector<Str*> strs;
	vector<int> code;
	int index, refs;
	uint globals, entry_point;
	bool released, built, optimize;
};
