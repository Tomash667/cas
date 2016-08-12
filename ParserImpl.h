#pragma once

#include "Op.h"

enum GROUP
{
	G_KEYWORD,
	G_VAR,
	G_CONST
};

enum KEYWORD
{
	K_IF,
	K_ELSE,
	K_DO,
	K_WHILE,
	K_FOR,
	K_BREAK,
	K_RETURN,
	K_CLASS,
	K_IS
};

enum CONST
{
	C_TRUE,
	C_FALSE
};

enum FOUND
{
	F_NONE,
	F_VAR,
	F_FUNC,
	F_USER_FUNC,
	F_MEMBER
};

enum PseudoOp
{
	PUSH_BOOL = MAX_OP,
	NOP,
	OBJ_MEMBER,
	OBJ_FUNC,
	BREAK,
	RETURN,
	INTERNAL_GROUP,
	SPECIAL_OP, // op code below don't apply child nodes
	IF = SPECIAL_OP,
	DO_WHILE,
	WHILE,
	FOR,
	GROUP
};

enum PseudoOpValue
{
	DO_WHILE_NORMAL = 0,
	DO_WHILE_ONCE = 1,
	DO_WHILE_INF = 2
};

enum RefType
{
	REF_NO,
	REF_MAY,
	REF_YES
};

struct ParseVar : ObjectPoolProxy<ParseVar>
{
	enum Type
	{
		LOCAL,
		GLOBAL,
		ARG,
		MEMBER
	};

	string name;
	int index;
	VarType type;
	Type subtype;

	inline cstring GetName()
	{
		return Format("%s %s", types[type.core]->name.c_str(), name.c_str());
	}
};

struct ParseNode : ObjectPoolProxy<ParseNode>
{
	union
	{
		Op op;
		PseudoOp pseudo_op;
	};
	union
	{
		bool bvalue;
		int value;
		float fvalue;
		string* str;
	};
	int type;
	vector<ParseNode*> childs;
	RefType ref;

	inline void OnFree() { SafeFree(childs); }

	inline void push(ParseNode* p) { childs.push_back(p); }
	inline void push(vector<ParseNode*>& ps)
	{
		for(ParseNode* p : ps)
			childs.push_back(p);
	}
	inline void push(Op op)
	{
		ParseNode* node = ParseNode::Get();
		node->op = op;
		push(node);
	}
	inline void push(Op op, int value)
	{
		ParseNode* node = ParseNode::Get();
		node->op = op;
		node->value = value;
		push(node);
	}
	inline VarType GetVarType()
	{
		return VarType(type, ref == REF_YES ? SV_REF : SV_NORMAL);
	}
};

struct Block : ObjectPoolProxy<Block>
{
	Block* parent;
	vector<Block*> childs;
	vector<ParseVar*> vars;
	uint var_offset;

	uint GetMaxVars() const
	{
		uint count = vars.size();
		uint top = 0u;
		for(const Block* b : childs)
		{
			uint count2 = b->GetMaxVars();
			if(count2 > top)
				top = count2;
		}
		return count + top;
	}

	ParseVar* FindVarSingle(const string& name) const
	{
		for(ParseVar* v : vars)
		{
			if(v->name == name)
				return v;
		}
		return nullptr;
	}

	ParseVar* FindVar(const string& name) const
	{
		const Block* block = this;

		while(block)
		{
			ParseVar* var = block->FindVarSingle(name);
			if(var)
				return var;
			block = block->parent;
		}

		return nullptr;
	}

	ParseVar* GetVar(uint index) const
	{
		for(ParseVar* v : vars)
		{
			if(v->index == index)
				return v;
		}
		if(parent)
			return GetVar(index);
		assert(0);
		return nullptr;
	}
};

struct ParseFunction : CommonFunction
{
	uint pos;
	uint locals;
	ParseNode* node;
	Block* block;
	vector<ParseVar*> args;

	ParseVar* FindArg(const string& name)
	{
		for(ParseVar* arg : args)
		{
			if(arg->name == name)
				return arg;
		}
		return nullptr;
	}
};

union Found
{
	ParseVar* var;
	Function* func;
	ParseFunction* ufunc;
	struct
	{
		Member* member;
		int member_index;
	};

	inline cstring ToString(FOUND type)
	{
		switch(type)
		{
		case F_VAR:
			switch(var->subtype)
			{
			case ParseVar::LOCAL:
				return "local variable";
			case ParseVar::GLOBAL:
				return "global variable";
			case ParseVar::ARG:
				return "argument";
			default:
				assert(0);
				return "undefined variable";
			}
		case F_FUNC:
			return "function";
		case F_USER_FUNC:
			return "script function";
		case F_MEMBER:
			return "member";
		default:
			assert(0);
			return "undefined";
		}
	}
};
