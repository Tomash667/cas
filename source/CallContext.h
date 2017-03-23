#pragma once

#include "cas/IObject.h"
#include "ICallContextProxy.h"
#include "Var.h"
#include "VectorOffset.h"
#include "StackFrame.h"

class Module;
struct Arg;
struct CodeFunction;
struct Member;
struct Object;
struct Str;

// Call context runs script code
class CallContext final : public ICallContextProxy
{
public:
	CallContext(int index, Module& module, cstring name);
	~CallContext();

	// from ICallContext
	cas::IObject* CreateInstance(cas::IType* type) override;
	vector<string>& GetAsserts() override;
	std::pair<cstring, int> GetCurrentLocation() override;
	std::pair<cas::IFunction*, cas::IObject*> GetEntryPoint() override;
	cstring GetException() override;
	cas::IObject* GetGlobal(cas::IGlobal* global) override;
	cas::IModule* GetModule() override;
	cstring GetName() override;
	cas::Value GetReturnValue() override;
	void PushValue(const cas::Value& val) override;
	void Release() override;
	bool Run() override;
	bool SetEntryPoint(cas::IFunction* func) override;
	bool SetEntryPointObj(cas::IFunction* func, cas::IObject* obj) override;
	void SetName(cstring name) override;

	// from ICallContextProxy
	void AddAssert(cstring msg) override;
	void ReleaseClass(Class* c) override;

	int GetIndex() const { return index; }

private:
	struct GetRefData
	{
		int* data, *real_data;
		VarType vartype;
		bool is_code, ref_to_class;

		GetRefData(int* data, VarType vartype, bool is_code = false, bool ref_to_class = false) : data(data), vartype(vartype), is_code(is_code),
			ref_to_class(ref_to_class)
		{
			if(vartype.type == V_STRING && !is_code)
				real_data = (int*)*(Str**)data; // dereference Str** to Str*
			else
				real_data = data;
		}

		template<typename T>
		T& as()
		{
			return *(T*)data;
		}
	};

	void AddRef(Var& v);
	void Callu(ScriptFunction& f, StackFrame::Type type);
	void Cast(Var& v, VarType vartype);
	void CleanupReturnValue();
	void ConvertReturnValue(VarType* expected);
	void ExecuteFunction(CodeFunction& f);
	void ExecuteSimpleFunction(CodeFunction& f, void* _this);
	GetRefData GetRef(Var& v);
	void MakeSingleInstance(Var& v);
	bool MatchFunctionCall(Function& f);
	void PushFunctionDefaults(Function& f);
	void PushStackFrame(StackFrame::Type type, uint pos, uint expected_stack);
	void ReleaseRef(Var& v);
	void RunInternal();
	void SetFromStack(VectorOffset<Var>& vo);
	void SetMemberValue(Class* c, Member* m, Var& v);
	VarType TypeToVarType(cas::Type& type);
	void ValuesToStack();
	bool VerifyFunctionArg(Var& v, Arg& arg);
	bool VerifyFunctionEntryPoint(Function* f, Object* obj);

	const uint MAX_STACK_DEPTH = 100u;

	vector<Var> stack, global, local;
	vector<StackFrame> stack_frames;
	vector<RefVar*> refs;
	vector<string> asserts;
	Module& module;
	Function* entry_point;
	Object* entry_point_obj;
	Str* retval_str;
	string name, exc;
	Var tmpv;
	cas::Value return_value;
	uint depth;
	int index, current_function, args_offset, locals_offset, current_line;
	vector<int>* code;
	vector<cas::Value> values;
	int* code_pos;
	int cleanup_offset;
};
