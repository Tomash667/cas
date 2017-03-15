#include "Pch.h"
#include "Decompiler.h"
#include "Module.h"
#include "Op.h"
#include "Str.h"
#include "VarType.h"

enum EXT_VAR_TYPE
{
	V_CODE_FUNCTION = V_MAX,
	V_SCRIPT_FUNCTION
};

struct OpInfo
{
	Op op;
	cstring name;
	int arg1, arg2;
};

#define Args0() V_VOID, V_VOID
#define Args1(arg1) arg1, V_VOID
#define Args2(arg1, arg2) arg1, arg2

OpInfo ops[] = {
	NOP, "nop", Args0(),
	PUSH, "push", Args0(),
	PUSH_TRUE, "push_true", Args0(),
	PUSH_FALSE, "push_false", Args0(),
	PUSH_CHAR, "push_char", Args1(V_CHAR),
	PUSH_INT, "push_int", Args1(V_INT),
	PUSH_FLOAT, "push_float", Args1(V_FLOAT),
	PUSH_STRING, "push_string", Args1(V_STRING),
	PUSH_LOCAL, "push_local", Args1(V_INT),
	PUSH_LOCAL_REF, "push_local_ref", Args1(V_INT),
	PUSH_GLOBAL, "push_global", Args1(V_INT),
	PUSH_GLOBAL_REF, "push_global_ref", Args1(V_INT),
	PUSH_ARG, "push_arg", Args1(V_INT),
	PUSH_ARG_REF, "push_arg_ref", Args1(V_INT),
	PUSH_MEMBER, "push_member", Args1(V_INT),
	PUSH_MEMBER_REF, "push_member_ref", Args1(V_INT),
	PUSH_THIS_MEMBER, "push_this_member", Args1(V_INT),
	PUSH_THIS_MEMBER_REF, "push_this_member_ref", Args1(V_INT),
	PUSH_TMP, "push_tmp", Args0(),
	PUSH_INDEX, "push_index", Args0(),
	PUSH_THIS, "push_this", Args0(),
	PUSH_ENUM, "push_enum", Args2(V_TYPE, V_INT),
	POP, "pop", Args0(),
	SET_LOCAL, "set_local", Args1(V_INT),
	SET_GLOBAL, "set_global", Args1(V_INT),
	SET_ARG, "set_arg", Args1(V_INT),
	SET_MEMBER, "set_member", Args1(V_INT),
	SET_THIS_MEMBER, "set_this_member", Args1(V_INT),
	SET_TMP, "set_tmp", Args0(),
	SWAP, "swap", Args1(V_INT),
	CAST, "cast", Args1(V_TYPE),
	NEG, "neg", Args0(),
	ADD, "add", Args0(),
	SUB, "sub", Args0(),
	MUL, "mul", Args0(),
	DIV, "div", Args0(),
	MOD, "mod", Args0(),
	BIT_AND, "bit_and", Args0(),
	BIT_OR, "bit_or", Args0(),
	BIT_XOR, "bit_xor", Args0(),
	BIT_LSHIFT, "bit_lshift", Args0(),
	BIT_RSHIFT, "bit_rshift", Args0(),
	INC, "inc", Args0(),
	DEC, "dec", Args0(),
	DEREF, "deref", Args0(),
	SET_ADR, "set_adr", Args0(),
	IS, "is", Args0(),
	EQ, "eq", Args0(),
	NOT_EQ, "not_eq", Args0(),
	GR, "gr", Args0(),
	GR_EQ, "gr_eq", Args0(),
	LE, "le", Args0(),
	LE_EQ, "le_eq", Args0(),
	AND, "and", Args0(),
	OR, "or", Args0(),
	NOT, "not",Args0(),
	BIT_NOT, "bit_not", Args0(),
	JMP, "jmp", Args1(V_INT),
	TJMP, "tjmp", Args1(V_INT),
	FJMP, "fjmp", Args1(V_INT),
	CALL, "call", Args1(V_CODE_FUNCTION),
	CALLU, "callu", Args1(V_SCRIPT_FUNCTION),
	CALLU_CTOR, "callu_ctor", Args1(V_SCRIPT_FUNCTION),
	RET, "ret", Args0(),
	IRET, "iret", Args0(),
	COPY, "copy", Args0(),
	COPY_ARG, "copy_arg", Args1(V_INT),
	RELEASE_REF, "release_ref", Args1(V_INT),
	RELEASE_OBJ, "release_obj", Args1(V_INT),
	LINE, "line", Args1(V_INT)
};
static_assert(sizeof(ops) / sizeof(OpInfo) == MAX_OP, "Missing decompile op codes.");

Decompiler::Decompiler() : s_output(nullptr), decompile_marker(false)
{

}

void Decompiler::Init(cas::Settings& settings)
{
	s_output = (std::ostream*)settings.output;
	decompile_marker = settings.decompile_marker;
}

void Decompiler::Decompile(Module& _module)
{
	assert(s_output);

	module = &_module;
	std::ostream& out = *s_output;

	if(decompile_marker)
		out << "***DCMP***";

	out << "DECOMPILE:\n";
	
	DecompileFunctions();
	DecompileMain();

	out << "\n";
	if(decompile_marker)
		out << "***DCMP_END***";
}

void Decompiler::DecompileFunctions()
{
	auto& code = module->GetFunctionsBytecode();
	auto& funcs = module->GetScriptFunctions();
	if(funcs.empty())
		return;

	for(uint i = 0, count = funcs.size(); i < count; ++i)
	{
		ScriptFunction* f = funcs[i];
		*s_output << Format("%s ", f->GetFormattedName(false));
		uint next = (i + 1 == funcs.size() ? code.size() : funcs[i + 1]->pos);
		DecompileBlock(code.data() + f->pos, code.data() + next);
	}
}

void Decompiler::DecompileMain()
{
	auto& code = module->GetBytecode();
	if(code.empty())
		return;
	*s_output << "Main:\n";
	DecompileBlock(code.data(), code.data() + code.size());
}

void Decompiler::DecompileBlock(int* start, int* end)
{
	std::ostream& out = *s_output;
	int* c = start;
	while(c != end)
	{
		Op op = (Op)*c++;
		if(op >= MAX_OP)
			out << "\tMISSING (" << op << ")\n";
		else
		{
			OpInfo& opi = ops[op];
			bool have_arg1 = false, have_arg2 = false;
			int arg1, arg2;
			if(opi.arg1 != V_VOID)
			{
				have_arg1 = true;
				arg1 = *c++;
				if(opi.arg2 != V_VOID)
				{
					have_arg2 = true;
					arg2 = *c++;
				}
			}

			if(have_arg1)
			{
				if(have_arg2)
				{
					out << Format("\t[%d %d %d] %s ", (int)op, arg1, arg2, opi.name);
					DecompileType(opi.arg1, arg1);
					DecompileType(opi.arg2, arg2);
					out << '\n';
				}
				else
				{
					out << Format("\t[%d %d] %s ", (int)op, arg1, opi.name);
					DecompileType(opi.arg1, arg1);
					out << '\n';
				}
			}
			else
				out << Format("\t[%d] %s\n", (int)op, opi.name);
		}
	}
}

void Decompiler::DecompileType(int type, int val)
{
	std::ostream& out = *s_output;

	switch(type)
	{
	case V_CHAR:
		out << Format("'%s' ", EscapeChar(union_cast<char>(val)));
		break;
	case V_INT:
		out << Format("%d ", val);
		break;
	case V_FLOAT:
		out << Format("%.2g ", union_cast<float>(val));
		break;
	case V_STRING:
		out << Format("\"%s\" ", Escape(module->GetStr(val)->s.c_str()));
		break;
	case V_CODE_FUNCTION:
		out << Format("%s ", module->GetCodeFunction(val)->GetFormattedName(false));
		break;
	case V_SCRIPT_FUNCTION:
		out << Format("%s ", module->GetScriptFunction(val)->GetFormattedName(false));
		break;
	case V_TYPE:
		out << Format("%s ", module->GetType(val)->name.c_str());
		break;
	default:
		assert(0);
		break;
	}
}

