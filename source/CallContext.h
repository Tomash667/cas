#pragma once

#include "ICallContextProxy.h"
#include "Var.h"
#include "VectorOffset.h"

class Module;
struct ArgInfo;
struct Function;
struct Member;
struct StackFrame;
struct Str;

// Call context runs script code
class CallContext : public ICallContextProxy
{
public:
	CallContext(int index, Module& module, cstring name);

	// from ICallContext
	std::pair<cstring, int> GetCurrentLocation() override;
	cstring GetException() override;
	cstring GetName() override;
	cas::ReturnValue GetReturnValue() override;
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
	void ExecuteFunction(Function& f);
	void ExecuteSimpleFunction(Function& f, void* _this);
	GetRefData GetRef(Var& v);
	void MakeSingleInstance(Var& v);
	void PushStackFrame(StackFrame& frame);
	void ReleaseRef(Var& v);
	void RunInternal();
	void SetFromStack(VectorOffset<Var>& vo);
	void SetMemberValue(Class* c, Member* m, Var& v);
	bool VerifyFunctionArg(Var& v, ArgInfo& arg);

	const uint MAX_STACK_DEPTH = 100u;

	vector<Var> stack, global, local;
	vector<StackFrame> stack_frames;
	vector<RefVar*> refs;
	vector<string> asserts;
	Module& module;
	Str* retval_str;
	string name, exc;
	Var tmpv;
	cas::ReturnValue return_value;
	uint depth;
	int index, current_function, args_offset, locals_offset, current_line;
	int* code_start;
	int* code_end;
	int* code_pos;
	int cleanup_offset;
};
