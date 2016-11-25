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
	K_STRUCT,
	K_IS,
	K_AS,
	K_OPERATOR,
	K_SWITCH,
	K_CASE,
	K_DEFAULT,
	K_IMPLICIT
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
	PUSH_TYPE,
	NOP,
	OBJ_MEMBER,
	OBJ_FUNC,
	SUBSCRIPT,
	CALL_FUNCTOR,
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

enum SYMBOL
{
	S_ADD,
	S_SUB,
	S_MUL,
	S_DIV,
	S_MOD,
	S_BIT_AND,
	S_BIT_OR,
	S_BIT_XOR,
	S_BIT_LSHIFT,
	S_BIT_RSHIFT,
	S_PLUS,
	S_MINUS,
	S_EQUAL,
	S_NOT_EQUAL,
	S_GREATER,
	S_GREATER_EQUAL,
	S_LESS,
	S_LESS_EQUAL,
	S_AND,
	S_OR,
	S_NOT,
	S_BIT_NOT,
	S_MEMBER_ACCESS,
	S_ASSIGN,
	S_ASSIGN_ADD,
	S_ASSIGN_SUB,
	S_ASSIGN_MUL,
	S_ASSIGN_DIV,
	S_ASSIGN_MOD,
	S_ASSIGN_BIT_AND,
	S_ASSIGN_BIT_OR,
	S_ASSIGN_BIT_XOR,
	S_ASSIGN_BIT_LSHIFT,
	S_ASSIGN_BIT_RSHIFT,
	S_PRE_INC,
	S_PRE_DEC,
	S_POST_INC,
	S_POST_DEC,
	S_IS,
	S_AS,
	S_SUBSCRIPT,
	S_CALL,
	S_TERNARY,
	S_INVALID,
	S_MAX
};

enum SYMBOL_TYPE
{
	ST_NONE,
	ST_ASSIGN,
	ST_INC_DEC,
	ST_SUBSCRIPT,
	ST_CALL
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

enum BASIC_SYMBOL
{
	BS_PLUS, // +
	BS_MINUS, // -
	BS_MUL, // *
	BS_DIV, // /
	BS_MOD, // %
	BS_BIT_AND, // &
	BS_BIT_OR, // |
	BS_BIT_XOR, // ^
	BS_BIT_LSHIFT, // <<
	BS_BIT_RSHIFT, // >>
	BS_EQUAL, // ==
	BS_NOT_EQUAL, // !=
	BS_GREATER, // >
	BS_GREATER_EQUAL, // >=
	BS_LESS, // <
	BS_LESS_EQUAL, // <=
	BS_AND, // &&
	BS_OR, // ||
	BS_NOT, // !
	BS_BIT_NOT, // ~
	BS_ASSIGN, // =
	BS_ASSIGN_ADD, // +=
	BS_ASSIGN_SUB, // -=
	BS_ASSIGN_MUL, // *=
	BS_ASSIGN_DIV, // /=
	BS_ASSIGN_MOD, // %=
	BS_ASSIGN_BIT_AND, // &=
	BS_ASSIGN_BIT_OR, // |=
	BS_ASSIGN_BIT_XOR, // ^=
	BS_ASSIGN_BIT_LSHIFT, // <<=
	BS_ASSIGN_BIT_RSHIFT, // >>=
	BS_DOT, // .
	BS_INC, // ++
	BS_DEC, // --
	BS_IS, // is
	BS_AS, // as
	BS_SUBSCRIPT, // [
	BS_CALL, // (
	BS_TERNARY, // ?
	BS_MAX
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
};

struct ReturnStructVar : VarSource
{
	ParseNode* node;
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

	inline ParseNode* copy()
	{
		ParseNode* p = Get();
		p->op = op;
		p->value = value;
		p->result = result;
		p->source = source;
		if(!childs.empty())
		{
			p->childs.reserve(childs.size());
			for(ParseNode* c : childs)
				p->childs.push_back(c->copy());
		}
		return p;
	}

	inline void OnFree() { SafeFree(childs); }

	inline void push(ParseNode* p) { childs.push_back(p); }
	inline void push(vector<ParseNode*>& ps)
	{
		childs.reserve(childs.size() + ps.size());
		for(ParseNode* p : ps)
			childs.push_back(p);
	}
	inline void push_copy(vector<ParseNode*>& ps)
	{
		childs.reserve(childs.size() + ps.size());
		for(ParseNode* p : ps)
			childs.push_back(p->copy());
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
};

struct Block : ObjectPoolProxy<Block>
{
	Block* parent;
	vector<Block*> childs;
	vector<ParseVar*> vars;
	uint var_offset;

	inline void OnFree()
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

struct ParseFunction : CommonFunction
{
	uint pos;
	uint locals;
	ParseNode* node;
	Block* block;
	vector<ParseVar*> args;

	ParseFunction() : node(nullptr), block(nullptr)
	{

	}

	~ParseFunction()
	{
		if(node)
			node->Free();
		if(block)
			block->Free();
		ParseVar::Free(args);
	}

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

struct SymbolInfo
{
	SYMBOL symbol;
	cstring name;
	int priority;
	bool left_associativity;
	int args, op;
	SYMBOL_TYPE type;
	cstring op_code;
	cstring oper;
};

struct SymbolNode
{
	ParseNode* node;
	SYMBOL symbol;
	bool is_symbol;

	inline SymbolNode(ParseNode* node) : node(node), is_symbol(false) {}
	inline SymbolNode(SYMBOL symbol, ParseNode* node = nullptr) : symbol(symbol), node(node), is_symbol(true) {}
};

struct BasicSymbolInfo
{
	BASIC_SYMBOL symbol;
	cstring text;
	SYMBOL pre_symbol;
	SYMBOL post_symbol;
	SYMBOL op_symbol;
	cstring full_over_text;

	inline SYMBOL operator [](int index)
	{
		switch(index)
		{
		default:
		case 0:
			return pre_symbol;
		case 1:
			return post_symbol;
		case 2:
			return op_symbol;
		}
	}

	inline cstring GetOverloadText()
	{
		return full_over_text ? full_over_text : text;
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

	inline CastResult() : type(CANT), ref_type(NO), cast_func(nullptr), ctor_func(nullptr)
	{

	}

	inline bool CantCast()
	{
		return type == CANT;
	}

	inline bool NeedCast()
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
