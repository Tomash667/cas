#pragma once

#include "ICallContextProxy.h"
#include "Var.h"
#include "VectorOffset.h"

class Module;
struct Arg;
struct CodeFunction;
struct Member;
struct StackFrame;
struct Str;

// Call context runs script code
class CallContext : public ICallContextProxy
{
public:
	CallContext(int index, Module& module, cstring name);
	~CallContext();

	// from ICallContext
	vector<string>& GetAsserts() override;
	std::pair<cstring, int> GetCurrentLocation() override;
	cstring GetException() override;
	cas::IObject* GetGlobal(cas::IGlobal* global) override;
	cas::IModule* GetModule() override;
	cstring GetName() override;
	cas::Value GetReturnValue() override;
	void Release() override;
	bool Run() override;
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
	void Cast(Var& v, VarType vartype);
	void CleanupReturnValue();
	void ExecuteFunction(CodeFunction& f);
	void ExecuteSimpleFunction(CodeFunction& f, void* _this);
	GetRefData GetRef(Var& v);
	void MakeSingleInstance(Var& v);
	void PushStackFrame(StackFrame& frame);
	void ReleaseRef(Var& v);
	void RunInternal();
	void SetFromStack(VectorOffset<Var>& vo);
	void SetMemberValue(Class* c, Member* m, Var& v);
	bool VerifyFunctionArg(Var& v, Arg& arg);

	const uint MAX_STACK_DEPTH = 100u;

	vector<Var> stack, global, local;
	vector<StackFrame> stack_frames;
	vector<RefVar*> refs;
	vector<string> asserts;
	Module& module;
	Str* retval_str;
	string name, exc;
	Var tmpv;
	cas::Value return_value;
	uint depth;
	int index, current_function, args_offset, locals_offset, current_line;
	int* code_start;
	int* code_end;
	int* code_pos;
	int cleanup_offset;
};
