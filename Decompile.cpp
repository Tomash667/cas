#include "Pch.h"
#include "Base.h"
#include "CasImpl.h"
#include "Type.h"
#include "Op.h"
#include "RunModule.h"
#include "Parser.h"
#include "Module.h"

enum EXT_VAR_TYPE
{
	V_FUNCTION = V_SPECIAL + 1,
	V_USER_FUNCTION,
	V_TYPE
};

struct OpInfo
{
	Op op;
	cstring name;
	int arg1;
};

OpInfo ops[MAX_OP] = {
	PUSH, "push", V_VOID,
	PUSH_TRUE, "push_true", V_VOID,
	PUSH_FALSE, "push_false", V_VOID,
	PUSH_INT, "push_int", V_INT,
	PUSH_FLOAT, "push_float", V_FLOAT,
	PUSH_STRING, "push_string", V_STRING,
	PUSH_LOCAL, "push_local", V_INT,
	PUSH_LOCAL_REF, "push_local_ref", V_INT,
	PUSH_GLOBAL, "push_global", V_INT,
	PUSH_GLOBAL_REF, "push_global_ref", V_INT,
	PUSH_ARG, "push_arg", V_INT,
	PUSH_ARG_REF, "push_arg_ref", V_INT,
	PUSH_MEMBER, "push_member", V_INT,
	PUSH_MEMBER_REF, "push_member_ref", V_INT,
	PUSH_THIS_MEMBER, "push_this_member", V_INT,
	PUSH_THIS_MEMBER_REF, "push_this_member_ref", V_INT,
	POP, "pop", V_VOID,
	SET_LOCAL, "set_local", V_INT,
	SET_GLOBAL, "set_global", V_INT,
	SET_ARG, "set_arg", V_INT,
	SET_MEMBER, "set_member", V_INT,
	SET_THIS_MEMBER, "set_this_member", V_INT,
	CAST, "cast", V_TYPE,
	NEG, "neg", V_VOID,
	ADD, "add", V_VOID,
	SUB, "sub", V_VOID,
	MUL, "mul", V_VOID,
	DIV, "div", V_VOID,
	MOD, "mod", V_VOID,
	BIT_AND, "bit_and", V_VOID,
	BIT_OR, "bit_or", V_VOID,
	BIT_XOR, "bit_xor", V_VOID,
	BIT_LSHIFT, "bit_lshift", V_VOID,
	BIT_RSHIFT, "bit_rshift", V_VOID,
	INC, "inc", V_VOID,
	DEC, "dec", V_VOID,
	DEREF, "deref", V_VOID,
	SET_ADR, "set_adr", V_VOID,
	IS, "is", V_VOID,
	EQ, "eq", V_VOID,
	NOT_EQ, "not_eq", V_VOID,
	GR, "gr", V_VOID,
	GR_EQ, "gr_eq", V_VOID,
	LE, "le", V_VOID,
	LE_EQ, "le_eq", V_VOID,
	AND, "and", V_VOID,
	OR, "or", V_VOID,
	NOT, "not", V_VOID,
	BIT_NOT, "bit_not", V_VOID,
	JMP, "jmp", V_INT,
	TJMP, "tjmp", V_INT,
	FJMP, "fjmp", V_INT,
	CALL, "call", V_FUNCTION,
	CALLU, "callu", V_USER_FUNCTION,
	CALLU_CTOR, "callu_ctor", V_USER_FUNCTION,
	RET, "ret", V_VOID,
	CTOR, "ctor", V_TYPE
};

void Decompile(RunModule& module)
{
	Parser& parser = *module.parent->parser;
	int* c = module.code.data();
	int* end = c + module.code.size();
	int cf = -2;

	cout << "DECOMPILE:\n";
	while(c != end)
	{
		if(cf == -2)
		{
			if(module.ufuncs.empty())
			{
				cf = -1;
				cout << "Main:\n";
			}
			else
			{
				cf = 0;
				cout << "Function " << parser.GetParserFunctionName(0) << ":\n";
			}
		}
		else if(cf != -1)
		{
			uint offset = c - module.code.data();
			if(offset >= module.entry_point)
			{
				cf = -1;
				cout << "Main:\n";
			}
			else if(cf + 1 < (int)module.ufuncs.size())
			{
				if(offset >= module.ufuncs[cf + 1].pos)
				{
					++cf;
					cout << "Function " << parser.GetParserFunctionName(cf) << ":\n";
				}
			}
		}

		Op op = (Op)*c++;
		if(op >= MAX_OP)
			cout << "\tMISSING (" << op << ")\n";
		else
		{
			OpInfo& opi = ops[op];
			switch(opi.arg1)
			{
			case V_VOID:
				cout << Format("\t[%d] %s\n", (int)op, opi.name);
				break;
			case V_INT:
				{
					int value = *c++;
					cout << Format("\t[%d %d] %s %d\n", (int)op, value, opi.name, value);
				}
				break;
			case V_FLOAT:
				{
					int val = *c++;
					float value = union_cast<float>(val);
					cout << Format("\t[%d %d] %s %.2g\n", (int)op, val, opi.name, value);
				}
				break;
			case V_STRING:
				{
					int str_idx = *c++;
					cout << Format("\t[%d %d] %s \"%s\"\n", (int)op, str_idx, opi.name, module.strs[str_idx]->s.c_str());
				}
				break;
			case V_FUNCTION:
				{
					int f_idx = *c++;
					cout << Format("\t[%d %d] %s %s\n", (int)op, f_idx, opi.name, parser.GetName(parser.GetFunction(f_idx)));
				}
				break;
			case V_USER_FUNCTION:
				{
					int f_idx = *c++;
					cout << Format("\t[%d %d] %s %s\n", (int)op, f_idx, opi.name, parser.GetParserFunctionName(f_idx));
				}
				break;
			case V_TYPE:
				{
					int type = *c++;
					cout << Format("\t[%d %d] %s %s\n", (int)op, type, opi.name, parser.GetType(type)->name.c_str());
				}
				break;
			}
		}
	}
	cout << "\n";
}
