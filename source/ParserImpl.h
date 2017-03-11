#pragma once

#include "Op.h"
#include "Function.h"
#include "VarSource.h"
#include "VarType.h"

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
	K_STRUCT,
	K_IS,
	K_AS,
	K_OPERATOR,
	K_SWITCH,
	K_CASE,
	K_DEFAULT,
	K_IMPLICIT,
	K_DELETE,
	K_STATIC,
	K_ENUM
};

enum CONST
{
	C_TRUE,
	C_FALSE,
	C_THIS
};

enum FOUND
{
	F_NONE,
	F_VAR,
	F_FUNC,
	F_MEMBER
};

enum PseudoOp
{
	PUSH_BOOL = MAX_OP,
	PUSH_TYPE,
	OBJ_MEMBER,
	OBJ_FUNC,
	SUBSCRIPT,
	CALL_FUNCTOR,
	CALL_PARSE,
	CALL_PARSE_CTOR,
	BREAK,
	RETURN,
	INTERNAL_GROUP,
	SPECIAL_OP, // op code below don't apply child nodes
	IF = SPECIAL_OP,
	DO_WHILE,
	WHILE,
	FOR,
	GROUP,
	TERNARY_PART,
	TERNARY,
	SWITCH,
	CASE,
	DEFAULT_CASE,
	CASE_BLOCK
};

enum PseudoOpValue
{
	DO_WHILE_NORMAL = 0,
	DO_WHILE_ONCE = 1,
	DO_WHILE_INF = 2
};

enum LEFT
{
	LEFT_NONE,
	LEFT_SYMBOL,
	LEFT_UNARY,
	LEFT_ITEM,
	LEFT_PRE,
	LEFT_POST
};

struct ParseVar : VarSource, ObjectPoolProxy<ParseVar>
{
	enum Type
	{
		LOCAL,
		GLOBAL,
		ARG,
		MEMBER
	};

	string name;
	VarType vartype;
	Type subtype;
	int local_index;
	bool referenced;
};

struct ParseNode;

struct ReturnStructVar : VarSource
{
	ParseNode* node;
	bool code_result;
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
		char cvalue;
		int value;
		float fvalue;
		string* str;
	};
	VarType result;
	ParseNode* linked;
	vector<ParseNode*> childs;
	VarSource* source;
	bool owned;

	ParseNode* copy()
	{
		ParseNode* p = Get();
		p->op = op;
		p->value = value;
		p->result = result;
		p->source = source;
		p->owned = owned;
		if(!childs.empty())
		{
			p->childs.reserve(childs.size());
			for(ParseNode* c : childs)
				p->childs.push_back(c->copy());
		}
		return p;
	}

	void OnFree()
	{
		if(owned)
			SafeFree(childs);
		else
			childs.clear();
	}

	void push(ParseNode* p) { childs.push_back(p); }
	void push(vector<ParseNode*>& ps)
	{
		childs.reserve(childs.size() + ps.size());
		for(ParseNode* p : ps)
			childs.push_back(p);
	}
	void push_copy(vector<ParseNode*>& ps)
	{
		childs.reserve(childs.size() + ps.size());
		for(ParseNode* p : ps)
			childs.push_back(p->copy());
	}
	void push(Op op)
	{
		ParseNode* node = ParseNode::Get();
		node->op = op;
		push(node);
	}
	void push(Op op, int value)
	{
		ParseNode* node = ParseNode::Get();
		node->op = op;
		node->value = value;
		push(node);
	}
	VarType GetReturnType()
	{
		if(childs.empty())
			return V_VOID;
		else
			return childs.front()->result;
	}
};

struct Block : ObjectPoolProxy<Block>
{
	Block* parent;
	vector<Block*> childs;
	vector<ParseVar*> vars;
	uint var_offset;

	void OnFree()
	{
		Free(childs);
		ParseVar::Free(vars);
	}

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

struct ParseFunction : Function
{
	uint pos;
	uint locals;
	tokenizer::Pos start_pos;
	ParseNode* node;
	Block* block;
	vector<ParseVar*> arg_vars;

	ParseFunction() : node(nullptr), block(nullptr)
	{

	}

	~ParseFunction()
	{
		if(node)
			node->Free();
		if(block)
			block->Free();
		ParseVar::Free(arg_vars);
	}

	ParseVar* FindArg(const string& name)
	{
		for(ParseVar* arg : arg_vars)
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
	AnyFunction func;
	struct
	{
		Member* member;
		int member_index;
	};

	Found() : func() {}

	cstring ToString(FOUND type)
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
		case F_MEMBER:
			return "member";
		default:
			assert(0);
			return "undefined";
		}
	}
};

typedef ObjectPoolRef<Block> BlockRef;
typedef ObjectPoolRef<ParseNode> NodeRef;
typedef ObjectPoolRef<ParseVar> VarRef;

enum RETURN_INFO
{
	RI_NO,
	RI_YES,
	RI_BREAK
};

struct CastResult
{
	enum TYPE
	{
		CANT = 0,
		NOT_REQUIRED = 1,
		IMPLICIT_CAST = 2,
		EXPLICIT_CAST = 4,
		BUILTIN_CAST = 8,
		IMPLICIT_CTOR = 16
	};

	enum REF_TYPE
	{
		NO,
		DEREF,
		TAKE_REF
	};

	int type;
	REF_TYPE ref_type;
	AnyFunction cast_func;
	AnyFunction ctor_func;

	CastResult() : type(CANT), ref_type(NO), cast_func(nullptr), ctor_func(nullptr)
	{

	}

	bool CantCast()
	{
		return type == CANT;
	}

	bool NeedCast()
	{
		return type != NOT_REQUIRED || ref_type != NO;
	}
};

struct OpResult
{
	enum Result
	{
		NO,
		YES,
		CAST,
		OVERLOAD,
		FALLBACK
	};

	CastResult cast_result;
	VarType cast_var;
	VarType result_var;
	ParseNode* over_result;
	Result result;

	OpResult() : cast_var(V_VOID), result_var(V_VOID), over_result(nullptr), result(NO) {}
};
