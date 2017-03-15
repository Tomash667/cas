#pragma once

#include "Function.h"
#include "IModuleProxy.h"

class CallContext;
class Parser;
struct Global;
struct Str;

// Script module
// Container for code, types and functions
// Can have child and parent modules
class Module : public IModuleProxy
{
public:
	Module(int index, cstring name);
	~Module();

	// from IModule
	cas::IEnum* AddEnum(cstring type_name) override;
	bool AddFunction(cstring decl, const cas::FunctionInfo& func_info) override;
	bool AddParentModule(IModule* module) override;
	cas::IClass* AddType(cstring type_name, int size, int flags) override;
	cas::ICallContext* CreateCallContext(cstring name) override;
	void Decompile() override;
	cas::IFunction* GetFunction(cstring name_or_decl, int flags) override;
	void GetFunctionsList(vector<cas::IFunction*>& funcs, cstring name, int flags) override;
	cas::IGlobal* GetGlobal(cstring name, int flags) override;
	cstring GetName() override;
	const Options& GetOptions() override;
	cas::IType* GetType(cstring name, int flags) override;
	ParseResult Parse(cstring input) override;
	ParseResult ParseAndRun(cstring input) override;
	void QueryFunctions(delegate<bool(cas::IFunction*)> pred) override;
	void QueryGlobals(delegate<bool(cas::IGlobal*)> pred) override;
	void QueryTypes(delegate<bool(cas::IType*)> pred) override;
	void Release() override;
	void Reset() override;
	bool Run() override;
	void SetName(cstring name) override;
	void SetOptions(const Options& options) override;

	// from IModuleProxy
	bool AddEnumValue(Type* type, cstring name, int value);
	bool AddMember(Type* type, cstring decl, int offset);
	bool AddMethod(Type* type, cstring decl, const cas::FunctionInfo& func_info);
	cas::ComplexType GetComplexType(VarType vartype) override;
	bool GetFunctionDecl(cstring decl, string& real_decl, Type* type) override;
	cstring GetName(VarType vartype) override;
	Type* GetType(int index) override;

	Type* AddCoreType(cstring type_name, int size, CoreVarType var_type, int flags);
	AnyFunction FindEqualFunction(Function* f);
	AnyFunction FindFunction(const string& name);
	vector<int>& GetBytecode() { return code; }
	CodeFunction* GetCodeFunction(int index);
	vector<CodeFunction*>& GetCodeFunctions() { return code_funcs; }
	vector<int>& GetFunctionsBytecode() { return funcs_code; }
	vector<Global*>& GetGlobals() { return globals; }
	int GetIndex() { return index; }
	std::map<int, Module*>& GetModules() { return modules; }
	uint GetMainStackSize() { return main_stack_size; }
	ScriptFunction* GetScriptFunction(int index, bool local = false);
	vector<ScriptFunction*>& GetScriptFunctions() { return script_funcs; }
	Str* GetStr(int index);
	vector<Str*>& GetStrs() { return strs; }
	vector<Type*>& GetTypes() { return types; }
	void RemoveCallContext(CallContext* call_context);
	void SetMainStackSize(uint new_main_stack_size) { main_stack_size = new_main_stack_size; }

private:
	void AddParserType(Type* type);
	bool BuildModule();
	void Cleanup(bool dtor);
	cas::IFunction* GetFunctionInternal(cstring name, cstring decl, int flags);
	void RemoveRef();
	bool VerifyFlags(int flags) const;
	bool VerifyTypeName(cstring type_name) const;

	Parser* parser;
	string name;
	std::map<int, Module*> modules;
	vector<Module*> child_modules;
	vector<CallContext*> call_contexts;
	vector<CodeFunction*> code_funcs;
	vector<ScriptFunction*> script_funcs;
	vector<Global*> globals;
	vector<Type*> types;
	vector<Str*> strs;
	vector<int> code, funcs_code;
	Options options;
	int index, refs, call_context_counter;
	uint main_stack_size;
	bool released, built;
};
