#include "Pch.h"
#include "Base.h"
#include "Op.h"
#include "Var.h"
#include "Function.h"
#include "Parse.h"

#undef CONST

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
	K_RETURN
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
	F_USER_FUNC
};

enum PseudoOp
{
	GROUP = MAX_OP,
	PUSH_BOOL,
	NOP,
	IF,
	PRE_CALL,
	DO_WHILE,
	WHILE,
	FOR,
	BREAK,
	RETURN
};

enum PseudoOpValue
{
	DO_WHILE_NORMAL = 0,
	DO_WHILE_ONCE = 1,
	DO_WHILE_INF = 2
};

enum RefType
{
	NO_REF,
	MAY_REF,
	REF
};

struct ParseVar : ObjectPoolProxy<ParseVar>
{
	enum Type
	{
		LOCAL,
		GLOBAL,
		ARG
	};

	string name;
	int index;
	VAR_TYPE type;
	Type subtype;
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
		ParseVar* var;
	};
	VAR_TYPE type;
	vector<ParseNode*> childs;
	RefType ref;

	inline void push(ParseNode* p) { childs.push_back(p); }
	inline void OnFree() { SafeFree(childs); }
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

	ParseVar* FindVarSingle(const string& id) const
	{
		for(ParseVar* v : vars)
		{
			if(v->name == id)
				return v;
		}
		return nullptr;
	}

	ParseVar* FindVar(const string& id) const
	{
		const Block* block = this;

		while(block)
		{
			ParseVar* var = block->FindVarSingle(id);
			if(var)
				return var;
			block = block->parent;
		}

		return nullptr;
	}
};

struct ParseFunction : CommonFunction
{
	string name;
	uint pos;
	uint locals;
	ParseNode* node;
	Block* block;
	int index;
	vector<ParseVar*> args;

	ParseVar* FindArg(const string& id)
	{
		for(ParseVar* arg : args)
		{
			if(arg->name == id)
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
		default:
			assert(0);
			return "undefined";
		}
	}
};


static Tokenizer t;
static vector<Str*> strs;
static vector<ParseFunction*> ufuncs;
static Block* main_block;
static Block* current_block;
static ParseFunction* current_function;
static int breakable_block;
static bool optimize;


void ParseArgs(vector<ParseNode*>& nodes);
ParseNode* ParseBlock(ParseFunction* f = nullptr);
ParseNode* ParseExpr(char end, char end2 = 0);
ParseNode* ParseLineOrBlock();


FOUND FindItem(const string& id, Found& found)
{
	Function* func = FindFunction(id);
	if(func)
	{
		found.func = func;
		return F_FUNC;
	}

	for(ParseFunction* ufunc : ufuncs)
	{
		if(ufunc->name == id)
		{
			found.ufunc = ufunc;
			return F_USER_FUNC;
		}
	}

	ParseVar* var = current_block->FindVar(id);
	if(var)
	{
		found.var = var;
		return F_VAR;
	}

	if(current_function)
	{
		// find globals while in function
		var = main_block->FindVar(id);
		if(var)
		{
			found.var = var;
			return F_VAR;
		}

		// find args
		var = current_function->FindArg(id);
		if(var)
		{
			found.var = var;
			return F_VAR;
		}
	}

	return F_NONE;
}

void CheckFindItem(const string& id)
{
	Found found;
	FOUND found_type = FindItem(id, found);
	if(found_type != F_NONE)
		t.Throw("Name '%s' already used as %s.", id.c_str(), found.ToString(found_type));
}

bool TryConstCast(ParseNode* node, VAR_TYPE type)
{
	if(type == V_STRING)
		return false;

	if(node->op == PUSH_BOOL)
	{
		if(type == V_INT)
		{
			// bool -> int
			node->value = (node->bvalue ? 1 : 0);
			node->op = PUSH_INT;
			node->type = V_INT;
		}
		else if(type == V_FLOAT)
		{
			// bool -> float
			node->fvalue = (node->bvalue ? 1.f : 0.f);
			node->op = PUSH_FLOAT;
			node->type = V_FLOAT;
		}
		else
			assert(0);
		return true;
	}
	else if(node->op == PUSH_INT)
	{
		if(type == V_BOOL)
		{
			// int -> bool
			node->bvalue = (node->value != 0);
			node->pseudo_op = PUSH_BOOL;
			node->type = V_BOOL;
		}
		else if(type == V_FLOAT)
		{
			// int -> float
			node->fvalue = (float)node->value;
			node->op = PUSH_FLOAT;
			node->type = V_FLOAT;
		}
		else
			assert(0);
		return true;
	}
	else if(node->op == PUSH_FLOAT)
	{
		if(type == V_BOOL)
		{
			// float -> bool
			node->bvalue = (node->fvalue != 0.f);
			node->pseudo_op = PUSH_BOOL;
			node->type = V_BOOL;
		}
		else if(type == V_INT)
		{
			// float -> int
			node->value = (int)node->fvalue;
			node->op = PUSH_INT;
			node->type = V_INT;
		}
		else
			assert(0);
		return true;
	}
	else
		return false;
}

void Cast(ParseNode*& node, VAR_TYPE type)
{
	// no cast required?
	if(type == V_VOID || (node->type == type && node->ref != REF))
		return;

	// can const cast?
	if(TryConstCast(node, type))
		return;

	if(node->ref == REF)
	{
		// dereference
		ParseNode* deref = ParseNode::Get();
		deref->op = DEREF;
		deref->type = node->type;
		deref->ref = NO_REF;
		deref->push(node);
		node = deref;

		if(node->type == type)
			return;
	}

	// normal cast
	ParseNode* cast = ParseNode::Get();
	cast->op = CAST;
	cast->value = type;
	cast->type = type;
	cast->ref = NO_REF;
	cast->push(node);
	node = cast;
}

// used in var assignment, passing argument to function
bool TryCast(ParseNode*& node, VAR_TYPE type)
{
	// no cast required?
	if(node->type == type)
		return true;

	// can't cast from void
	if(node->type == V_VOID)
		return false;

	// no implicit cast from string to bool/int/float
	if(node->type == V_STRING && (type == V_BOOL || type == V_INT || type == V_FLOAT))
		return false;

	Cast(node, type);
	return true;
}

void VerifyFunctionCall(ParseNode* node, CommonFunction* f, cstring name)
{
	// check params count
	if(node->childs.size() > f->arg_infos.size() || node->childs.size() < f->required_args)
		t.Throw("Function %s with %d arguments not found, function have %d arguments.", name, node->childs.size(), f->arg_infos.size());

	// verify params
	for(uint i = 0; i < node->childs.size(); ++i)
	{
		if(!TryCast(node->childs[i], f->arg_infos[i].type))
			t.Throw("Function %s takes %s for argument %d, found %s.", name, var_info[f->arg_infos[i].type].name, i + 1, var_info[node->childs[i]->type].name);
	}

	// fill default params
	for(uint i = node->childs.size(); i < f->arg_infos.size(); ++i)
	{
		ArgInfo& arg = f->arg_infos[i];
		ParseNode* n = ParseNode::Get();
		n->type = arg.type;
		switch(arg.type)
		{
		case V_BOOL:
			n->pseudo_op = PUSH_BOOL;
			n->bvalue = arg.bvalue;
			break;
		case V_INT:
			n->op = PUSH_INT;
			n->value = arg.value;
			break;
		case V_FLOAT:
			n->op = PUSH_FLOAT;
			n->fvalue = arg.fvalue;
			break;
		case V_STRING:
			n->op = PUSH_STRING;
			n->value = arg.value;
			break;
		default:
			assert(0);
			break;
		}
		node->push(n);
	}
}

ParseNode* ParseConstItem()
{
	if(t.IsInt())
	{
		// int
		int val = t.GetInt();
		ParseNode* node = ParseNode::Get();
		node->op = PUSH_INT;
		node->type = V_INT;
		node->value = val;
		node->ref = NO_REF;
		t.Next();
		return node;
	}
	else if(t.IsFloat())
	{
		// float
		float val = t.GetFloat();
		ParseNode* node = ParseNode::Get();
		node->op = PUSH_FLOAT;
		node->type = V_FLOAT;
		node->fvalue = val;
		node->ref = NO_REF;
		t.Next();
		return node;
	}
	else if(t.IsString())
	{
		// string
		int index = strs.size();
		Str* str = Str::Get();
		str->s = t.GetString();
		str->refs = 1;
		strs.push_back(str);
		ParseNode* node = ParseNode::Get();
		node->op = PUSH_STRING;
		node->value = index;
		node->type = V_STRING;
		node->ref = NO_REF;
		t.Next();
		return node;
	}
	else if(t.IsKeywordGroup(G_CONST))
	{
		CONST c = (CONST)t.GetKeywordId(G_CONST);
		t.Next();
		ParseNode* node = ParseNode::Get();
		node->pseudo_op = PUSH_BOOL;
		node->bvalue = (c == C_TRUE);
		node->type = V_BOOL;
		node->ref = NO_REF;
		return node;
	}
	else
		t.Unexpected();
}

ParseNode* ParseItem()
{
	if(t.IsItem())
	{
		const string& id = t.GetItem();
		Found found;
		FOUND found_type = FindItem(id, found);
		switch(found_type)
		{
		case F_VAR:
			{
				ParseVar* var = found.var;
				ParseNode* node = ParseNode::Get();
				switch(var->subtype)
				{
				default:
					assert(0);
				case ParseVar::LOCAL:
					node->op = PUSH_LOCAL;
					break;
				case ParseVar::GLOBAL:
					node->op = PUSH_GLOBAL;
					break;
				case ParseVar::ARG:
					node->op = PUSH_ARG;
					break;
				}
				node->type = var->type;
				node->var = var;
				node->ref = MAY_REF;
				t.Next();
				return node;
			}
		case F_FUNC:
			{
				Function* f = found.func;
				ParseNode* node = ParseNode::Get();
				node->op = CALL;
				node->type = f->result;
				node->value = f->index;
				node->ref = NO_REF;

				t.Next();
				ParseArgs(node->childs);
				VerifyFunctionCall(node, f, f->name);

				return node;
			}
		case F_USER_FUNC:
			{
				ParseFunction* f = found.ufunc;
				ParseNode* node = ParseNode::Get();
				node->op = CALLU;
				node->type = f->result;
				node->value = f->index;
				node->ref = NO_REF;

				t.Next();
				ParseArgs(node->childs);
				VerifyFunctionCall(node, f, f->name.c_str());

				return node;
			}
		default:
			assert(0);
		case F_NONE:
			t.Unexpected();
		}
	}
	else
		return ParseConstItem();
}

enum SYMBOL
{
	S_ADD,
	S_SUB,
	S_MUL,
	S_DIV,
	S_MOD,
	S_PLUS,
	S_MINUS,
	S_LEFT_PAR,
	S_EQUAL,
	S_NOT_EQUAL,
	S_GREATER,
	S_GREATER_EQUAL,
	S_LESS,
	S_LESS_EQUAL,
	S_AND,
	S_OR,
	S_NOT,
	S_MEMBER_ACCESS,
	S_ASSIGN,
	S_ASSIGN_ADD,
	S_ASSIGN_SUB,
	S_ASSIGN_MUL,
	S_ASSIGN_DIV,
	S_ASSIGN_MOD,
	S_PRE_INC,
	S_PRE_DEC,
	S_POST_INC,
	S_POST_DEC,
	S_INVALID,
	S_MAX
};

enum SYMBOL_TYPE
{
	ST_NONE,
	ST_ASSIGN,
	ST_INC_DEC
};

// http://en.cppreference.com/w/cpp/language/operator_precedence
struct SymbolInfo
{
	SYMBOL symbol;
	cstring name;
	int priority;
	bool left_associativity;
	int args, op;
	SYMBOL_TYPE type;
};

SymbolInfo symbols[S_MAX] = {
	S_ADD, "add", 6, true, 2, ADD, ST_NONE,
	S_SUB, "subtract", 6, true, 2, SUB, ST_NONE,
	S_MUL, "multiply", 5, true, 2, MUL, ST_NONE,
	S_DIV, "divide", 5, true, 2, DIV, ST_NONE,
	S_MOD, "modulo", 5, true, 2, MOD, ST_NONE,
	S_PLUS, "unary plus", 3, false, 1, NOP, ST_NONE,
	S_MINUS, "unary minus", 3, false, 1, NEG, ST_NONE,
	S_LEFT_PAR, "left parenthesis", 99, true, 0, NOP, ST_NONE,
	S_EQUAL, "equal", 9, true, 2, EQ, ST_NONE,
	S_NOT_EQUAL, "not equal", 9, true, 2, NOT_EQ, ST_NONE,
	S_GREATER, "greater", 8, true, 2, GR, ST_NONE,
	S_GREATER_EQUAL, "greater equal", 8, true, 2, GR_EQ, ST_NONE,
	S_LESS, "less", 8, true, 2, LE, ST_NONE,
	S_LESS_EQUAL, "less equal", 8, true, 2, LE_EQ, ST_NONE,
	S_AND, "and", 13, true, 2, AND, ST_NONE,
	S_OR, "or", 14, true, 2, OR, ST_NONE,
	S_NOT, "not", 3, false, 1, NOT, ST_NONE,
	S_MEMBER_ACCESS, "member access", 2, true, 2, NOP, ST_NONE,
	S_ASSIGN, "assign", 15, false, 2, NOP, ST_ASSIGN,
	S_ASSIGN_ADD, "assign add", 15, false, 2, S_ADD, ST_ASSIGN,
	S_ASSIGN_SUB, "assign subtract", 15, false, 2, S_SUB, ST_ASSIGN,
	S_ASSIGN_MUL, "assign multiply", 15, false, 2, S_MUL, ST_ASSIGN,
	S_ASSIGN_DIV, "assign divide", 15, false, 2, S_DIV, ST_ASSIGN,
	S_ASSIGN_MOD, "assign modulo", 15, false, 2, S_MOD, ST_ASSIGN,
	S_PRE_INC, "pre increment", 3, false, 1, PRE_INC, ST_INC_DEC,
	S_PRE_DEC, "pre decrement", 3, false, 1, PRE_DEC, ST_INC_DEC,
	S_POST_INC, "post increment", 2, true, 1, POST_INC, ST_INC_DEC,
	S_POST_DEC, "post decrement", 2, true, 1, POST_DEC, ST_INC_DEC,
	S_INVALID, "invalid", 99, true, 0, NOP, ST_NONE
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

bool CanOp(SYMBOL symbol, VAR_TYPE left, VAR_TYPE right, VAR_TYPE& cast, VAR_TYPE& result)
{
	if(left == V_VOID)
		return false;
	if(right == V_VOID && symbols[symbol].args != 1)
		return false;

	VAR_TYPE type;
	switch(symbol)
	{
	case S_ADD:
		if(left == V_STRING || right == V_STRING)
			type = V_STRING;
		else if(left == V_FLOAT || right == V_FLOAT)
			type = V_FLOAT;
		else // int or bool
			type = V_INT;
		cast = type;
		result = type;
		return true;
	case S_SUB:
	case S_MUL:
	case S_DIV:
	case S_MOD:
		if(left == V_STRING || right == V_STRING)
			return false; // can't do with string
		if(left == V_FLOAT || right == V_FLOAT)
			type = V_FLOAT;
		else // int or bool
			type = V_INT;
		cast = type;
		result = type;
		return true;
	case S_PLUS:
	case S_MINUS:
		if(left == V_INT || right == V_FLOAT)
		{
			cast = left;
			result = left;
			return true;
		}
		else if(left == V_BOOL)
		{
			cast = V_INT;
			result = V_INT;
			return true;
		}
		else
			return false;
	case S_EQUAL:
	case S_NOT_EQUAL:
	case S_GREATER:
	case S_GREATER_EQUAL:
	case S_LESS:
	case S_LESS_EQUAL:
		if(left == V_STRING || right == V_STRING)
		{
			if(symbol != S_EQUAL && symbol != S_NOT_EQUAL)
				return false;
			type = V_STRING;
		}
		else if(left == V_FLOAT || right == V_FLOAT)
			type = V_FLOAT;
		else if(left == V_INT || right == V_INT)
			type = V_INT;
		else if(symbol == S_EQUAL || symbol == S_NOT_EQUAL)
			type = V_INT;
		else
			type = V_BOOL;
		cast = type;
		result = V_BOOL;
		return true;
	case S_AND:
	case S_OR:
		// not allowed for string, cast other to bool
		if(left == V_STRING || right == V_STRING)
			return false;
		cast = V_BOOL;
		result = V_BOOL;
		return true;
	case S_NOT:
		// not allowed for string, cast other to bool
		if(left == V_STRING)
			return false;
		cast = V_BOOL;
		result = V_BOOL;
		return true;
	default:
		assert(0);
		return false;
	}
}

struct SymbolOrNode
{
	union
	{
		ParseNode* node;
		SYMBOL symbol;
	};
	bool is_symbol;

	inline SymbolOrNode(ParseNode* node) : node(node), is_symbol(false) {}
	inline SymbolOrNode(SYMBOL symbol) : symbol(symbol), is_symbol(true) {}
};

bool TryConstExpr1(ParseNode* node, SYMBOL symbol)
{
	if(symbol == S_PLUS)
		return true;
	else if(symbol == S_MINUS)
	{
		if(node->op == PUSH_INT)
		{
			node->value = -node->value;
			return true;
		}
		else if(node->op == PUSH_FLOAT)
		{
			node->fvalue = -node->fvalue;
			return true;
		}
	}
	else if(symbol == S_NOT)
	{
		if(node->op == PUSH_BOOL)
		{
			node->bvalue = !node->bvalue;
			return true;
		}
	}
	else
		assert(0);
	return false;
}

bool TryConstExpr(ParseNode* left, ParseNode* right, ParseNode* op, SYMBOL symbol)
{
	if(left->op != right->op)
		return false;

	if(left->op == PUSH_BOOL)
	{
		bool result;
		switch(symbol)
		{
		case S_EQUAL:
			result = (left->bvalue == right->bvalue);
			break;
		case S_NOT_EQUAL:
			result = (left->bvalue != right->bvalue);
			break;
		case S_AND:
			result = (left->bvalue && right->bvalue);
			break;
		case S_OR:
			result = (left->bvalue || right->bvalue);
			break;
		default:
			assert(0);
			result = false;
			break;
		}
		op->bvalue = result;
		op->type = V_BOOL;
		op->pseudo_op = PUSH_BOOL;
	}
	else if(left->op == PUSH_INT)
	{
		// optimize const int expr
		switch(symbol)
		{
		case S_ADD:
			op->value = left->value + right->value;
			op->op = PUSH_INT;
			break;
		case S_SUB:
			op->value = left->value - right->value;
			op->op = PUSH_INT;
			break;
		case S_MUL:
			op->value = left->value * right->value;
			op->op = PUSH_INT;
			break;
		case S_DIV:
			if(right->value == 0)
				op->value = 0;
			else
				op->value = left->value / right->value;
			op->op = PUSH_INT;
			break;
		case S_MOD:
			if(right->value == 0)
				op->value = 0;
			else
				op->value = left->value % right->value;
			op->op = PUSH_INT;
			break;
		case S_EQUAL:
			op->bvalue = (left->value == right->value);
			op->pseudo_op = PUSH_BOOL;
			break;
		case S_NOT_EQUAL:
			op->bvalue = (left->value != right->value);
			op->pseudo_op = PUSH_BOOL;
			break;
		case S_GREATER:
			op->bvalue = (left->value > right->value);
			op->pseudo_op = PUSH_BOOL;
			break;
		case S_GREATER_EQUAL:
			op->bvalue = (left->value >= right->value);
			op->pseudo_op = PUSH_BOOL;
			break;
		case S_LESS:
			op->bvalue = (left->value < right->value);
			op->pseudo_op = PUSH_BOOL;
			break;
		case S_LESS_EQUAL:
			op->bvalue = (left->value <= right->value);
			op->pseudo_op = PUSH_BOOL;
			break;
		default:
			assert(0);
			op->value = 0;
			op->op = PUSH_INT;
			break;
		}
	}
	else if(left->op == PUSH_FLOAT)
	{
		// optimize const float expr
		switch(symbol)
		{
		case S_ADD:
			op->fvalue = left->fvalue + right->fvalue;
			op->op = PUSH_FLOAT;
			break;
		case S_SUB:
			op->fvalue = left->fvalue - right->fvalue;
			op->op = PUSH_FLOAT;
			break;
		case S_MUL:
			op->fvalue = left->fvalue * right->fvalue;
			op->op = PUSH_FLOAT;
			break;
		case S_DIV:
			if(right->fvalue == 0.f)
				op->fvalue = 0.f;
			else
				op->fvalue = left->fvalue / right->fvalue;
			op->op = PUSH_FLOAT;
			break;
		case S_MOD:
			if(right->fvalue == 0.f)
				op->fvalue = 0.f;
			else
				op->fvalue = fmod(left->fvalue, right->fvalue);
			op->op = PUSH_FLOAT;
			break;
		case S_EQUAL:
			op->bvalue = (left->fvalue == right->fvalue);
			op->op = PUSH_FLOAT;
			break;
		case S_NOT_EQUAL:
			op->bvalue = (left->fvalue != right->fvalue);
			op->op = PUSH_FLOAT;
			break;
		case S_LESS:
			op->bvalue = (left->fvalue < right->fvalue);
			op->op = PUSH_FLOAT;
			break;
		case S_LESS_EQUAL:
			op->bvalue = (left->fvalue <= right->fvalue);
			op->op = PUSH_FLOAT;
			break;
		case S_GREATER:
			op->bvalue = (left->fvalue > right->fvalue);
			op->op = PUSH_FLOAT;
			break;
		case S_GREATER_EQUAL:
			op->bvalue = (left->fvalue >= right->fvalue);
			op->op = PUSH_FLOAT;
			break;
		default:
			assert(0);
			op->fvalue = 0;
			op->op = PUSH_FLOAT;
			break;
		}
	}
	else
		return false;

	left->Free();
	right->Free();
	return true;
}

void ParseArgs(vector<ParseNode*>& nodes)
{
	// (
	t.AssertSymbol('(');
	t.Next();

	// arguments
	if(!t.IsSymbol(')'))
	{
		while(true)
		{
			nodes.push_back(ParseExpr(',', ')'));
			if(t.IsSymbol(')'))
				break;
			t.AssertSymbol(',');
			t.Next();
		}
	}
	t.Next();
}

void RequireRef(ParseNode* node, cstring op_name)
{
	assert(node);
	if(node->ref == NO_REF)
		t.Throw("Need reference value for operation '%s'.", op_name);
	else if(node->ref == MAY_REF)
	{
		switch(node->op)
		{
		case PUSH_LOCAL:
			node->op = PUSH_LOCAL_REF;
			break;
		case PUSH_GLOBAL:
			node->op = PUSH_GLOBAL_REF;
			break;
		case PUSH_ARG:
			node->op = PUSH_ARG_REF;
			break;
		default:
			assert(0);
			break;
		}
		node->ref = REF;
	}
}

enum BASIC_SYMBOL
{
	BS_PLUS, // +
	BS_MINUS, // -
	BS_MUL, // *
	BS_DIV, // /
	BS_MOD, // %
	BS_EQUAL, // ==
	BS_NOT_EQUAL, // !=
	BS_GREATER, // >
	BS_GREATER_EQUAL, // >=
	BS_LESS, // <
	BS_LESS_EQUAL, // <=
	BS_AND, // &&
	BS_OR, // ||
	BS_NOT, // !
	BS_ASSIGN, // =
	BS_ASSIGN_ADD, // +=
	BS_ASSIGN_SUB, // -=
	BS_ASSIGN_MUL, // *=
	BS_ASSIGN_DIV, // /=
	BS_ASSIGN_MOD, // %=
	BS_DOT, // .
	BS_INC, // ++
	BS_DEC, // --
	BS_MAX
};

struct BasicSymbolInfo
{
	BASIC_SYMBOL symbol;
	cstring text;
	SYMBOL unary_symbol;
	SYMBOL pre_symbol;
	SYMBOL post_symbol;
	SYMBOL op_symbol;
};

BasicSymbolInfo basic_symbols[BS_MAX] = {
	BS_INC, "+", S_PLUS, S_INVALID, S_INVALID, S_ADD,
	BS_DEC, "-", S_MINUS, S_INVALID, S_INVALID, S_SUB,
	BS_MUL, "*", S_INVALID, S_INVALID, S_INVALID, S_MUL,
	BS_DIV, "/", S_INVALID, S_INVALID, S_INVALID, S_DIV,
	BS_MOD, "%", S_INVALID, S_INVALID, S_INVALID, S_MOD,
	BS_EQUAL, "==", S_INVALID, S_INVALID, S_INVALID, S_EQUAL,
	BS_NOT_EQUAL, "!=", S_INVALID, S_INVALID, S_INVALID, S_NOT_EQUAL,
	BS_GREATER, ">", S_INVALID, S_INVALID, S_INVALID, S_GREATER,
	BS_GREATER_EQUAL, ">=", S_INVALID, S_INVALID, S_INVALID, S_GREATER_EQUAL,
	BS_LESS, "<", S_INVALID, S_INVALID, S_INVALID, S_LESS,
	BS_LESS_EQUAL, "<=", S_INVALID, S_INVALID, S_INVALID, S_LESS_EQUAL,
	BS_AND, "&&", S_INVALID, S_INVALID, S_INVALID, S_AND,
	BS_OR, "||", S_INVALID, S_INVALID, S_INVALID, S_OR,
	BS_NOT, "!", S_NOT, S_INVALID, S_INVALID, S_INVALID,
	BS_ASSIGN, "=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN,
	BS_ASSIGN_ADD, "+=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_ADD,
	BS_ASSIGN_SUB, "-=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_SUB,
	BS_ASSIGN_MUL, "*=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_MUL,
	BS_ASSIGN_DIV, "/=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_DIV,
	BS_ASSIGN_MOD, "%=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_MOD,
	BS_DOT, ".", S_INVALID, S_INVALID, S_INVALID, S_MEMBER_ACCESS,
	BS_INC, "++", S_INVALID, S_PRE_INC, S_POST_INC, S_INVALID,
	BS_DEC, "--", S_INVALID, S_PRE_DEC, S_POST_DEC, S_INVALID,
};

BASIC_SYMBOL GetSymbol()
{
	char c = t.MustGetSymbol();
	switch(c)
	{
	case '+':
		if(t.PeekSymbol('='))
			return BS_ASSIGN_ADD;
		else if(t.PeekSymbol('+'))
			return BS_INC;
		else
			return BS_PLUS;
	case '-':
		if(t.PeekSymbol('='))
			return BS_ASSIGN_SUB;
		else if(t.PeekSymbol('-'))
			return BS_DEC;
		else
			return BS_MINUS;
	case '*':
		if(t.PeekSymbol('='))
			return BS_ASSIGN_MUL;
		else
			return BS_MUL;
	case '/':
		if(t.PeekSymbol('='))
			return BS_ASSIGN_DIV;
		else
			return BS_DIV;
	case '%':
		if(t.PeekSymbol('='))
			return BS_ASSIGN_MOD;
		else
			return BS_MOD;
	case '!':
		if(t.PeekSymbol('='))
			return BS_NOT_EQUAL;
		else
			return BS_NOT;
	case '=':
		if(t.PeekSymbol('='))
			return BS_EQUAL;
		else
			return BS_ASSIGN;
	case '>':
		if(t.PeekSymbol('='))
			return BS_GREATER_EQUAL;
		else
			return BS_GREATER;
	case '<':
		if(t.PeekSymbol('='))
			return BS_LESS_EQUAL;
		else
			return BS_LESS;
	case '&':
		if(!t.PeekSymbol('&'))
			t.Unexpected();
		return BS_AND;
	case '|':
		if(!t.PeekSymbol('|'))
			t.Unexpected();
		return BS_OR;
	case '.':
		return BS_DOT;
	default:
		return BS_MAX;
	}
}

bool GetNextSymbol(BASIC_SYMBOL& symbol)
{
	if(symbol != BS_MAX)
		return true;
	if(!t.IsSymbol())
		return false;
	symbol = GetSymbol();
	return (symbol != BS_MAX);
}

void PushSymbol(SYMBOL symbol, vector<SymbolOrNode>& exit, vector<SYMBOL>& stack)
{
	while(!stack.empty())
	{
		SYMBOL symbol2 = stack.back();
		SymbolInfo& s1 = symbols[symbol];
		SymbolInfo& s2 = symbols[symbol2];

		bool ok = false;
		if(s1.left_associativity)
			ok = (s1.priority >= s2.priority);
		else
			ok = (s1.priority > s2.priority);

		if(ok)
		{
			exit.push_back(symbol2);
			stack.pop_back();
		}
		else
			break;
	}

	stack.push_back(symbol);
}

BASIC_SYMBOL ParseExprPart(vector<SymbolOrNode>& exit, vector<SYMBOL>& stack)
{
	BASIC_SYMBOL symbol = BS_MAX;

	// [unary symbols]
	while(GetNextSymbol(symbol))
	{
		BasicSymbolInfo& bsi = basic_symbols[symbol];
		if(bsi.unary_symbol != S_INVALID)
		{
			PushSymbol(bsi.unary_symbol, exit, stack);
			t.Next();
			symbol = BS_MAX;
		}
		else
			break;
	}

	// [pre symbols]
	while(GetNextSymbol(symbol))
	{
		BasicSymbolInfo& bsi = basic_symbols[symbol];
		if(bsi.pre_symbol != S_INVALID)
		{
			PushSymbol(bsi.pre_symbol, exit, stack);
			t.Next();
			symbol = BS_MAX;
		}
		else
			break;
	}

	// item
	if(GetNextSymbol(symbol))
		t.Unexpected();
	if(t.IsSymbol('('))
	{
		t.Next();
		exit.push_back(ParseExpr(')'));
		t.Next();
	}
	else
		exit.push_back(ParseItem());

	// [post symbol]
	if(GetNextSymbol(symbol))
	{
		BasicSymbolInfo& bsi = basic_symbols[symbol];
		if(bsi.post_symbol != S_INVALID)
		{
			PushSymbol(bsi.post_symbol, exit, stack);
			t.Next();
			symbol = BS_MAX;
		}
	}

	return symbol;
}

ParseNode* ParseExpr(char end, char end2)
{
	vector<SymbolOrNode> exit;
	vector<SYMBOL> stack;

	while(true)
	{
		BASIC_SYMBOL left = ParseExprPart(exit, stack);
		next_symbol:
		if(GetNextSymbol(left))
		{
			BasicSymbolInfo& bsi = basic_symbols[left];
			if(bsi.op_symbol == S_INVALID)
				t.Unexpected();
			PushSymbol(bsi.op_symbol, exit, stack);
			t.Next();

			if(bsi.op_symbol == S_MEMBER_ACCESS)
			{
				string* str = StringPool.Get();
				*str = t.MustGetItem();
				t.Next();
				ParseNode* node = ParseNode::Get();
				node->pseudo_op = PRE_CALL;
				node->type = V_VOID;
				node->str = str;
				node->ref = NO_REF;
				ParseArgs(node->childs);
				exit.push_back(node);
				left = BS_MAX;
				goto next_symbol;
			}
		}
		else
			break;
	}

	while(!stack.empty())
	{
		exit.push_back(stack.back());
		stack.pop_back();
	}
	
	vector<ParseNode*> stack2;
	for(SymbolOrNode& sn : exit)
	{
		if(sn.is_symbol)
		{
			SymbolInfo& si = symbols[sn.symbol];
			if(si.args > (int)stack2.size())
				t.Throw("Missing arguments on stack for operator '%s'.", si.name);

			if(si.args == 1)
			{
				ParseNode* node = stack2.back();
				stack2.pop_back();
				if(si.type == ST_NONE)
				{
					VAR_TYPE cast, result;
					if(!CanOp(sn.symbol, node->type, V_VOID, cast, result))
						t.Throw("Invalid type '%s' for operation '%s'.", var_info[node->type].name, si.name);
					Cast(node, cast);
					if(!TryConstExpr1(node, si.symbol) && si.op != NOP)
					{
						ParseNode* op = ParseNode::Get();
						op->op = (Op)si.op;
						op->type = result;
						op->push(node);
						op->ref = NO_REF;
						node = op;
					}
					stack2.push_back(node);
				}
				else
				{
					assert(si.type == ST_INC_DEC);
					if(node->type != V_INT && node->type != V_FLOAT)
						t.Throw("Invalid type '%s' for operation '%s'.", var_info[node->type].name, si.name);
					RequireRef(node, si.name);

					ParseNode* op = ParseNode::Get();
					op->op = (Op)si.op;
					op->type = node->type;
					op->ref = ((si.symbol == S_PRE_INC || si.symbol == S_PRE_DEC) ? REF : NO_REF);
					op->push(node);
					stack2.push_back(op);
				}
			}
			else
			{
				assert(si.args == 2);
				ParseNode* right = stack2.back();
				stack2.pop_back();
				ParseNode* left = stack2.back();
				stack2.pop_back();

				if(si.symbol == S_MEMBER_ACCESS)
				{
					assert(right->op == PRE_CALL);
					if(left->type == V_VOID)
						t.Throw("Invalid member access for type 'void'.");
					Function* func = FindFunction(*right->str, left->type);
					if(!func)
						t.Throw("Missing function '%s' for type '%s'.", right->str->c_str(), var_info[left->type].name);
					StringPool.Free(right->str);
					VerifyFunctionCall(right, func, func->name);
					ParseNode* node = ParseNode::Get();
					node->op = CALL;
					node->type = func->result;
					node->value = func->index;
					node->ref = NO_REF;
					node->push(left);
					for(ParseNode* n : right->childs)
						node->push(n);
					right->Free();
					stack2.push_back(node);
				}
				else if(si.type == ST_ASSIGN)
				{
					if(left->op != PUSH_LOCAL && left->op != PUSH_GLOBAL)
						t.Throw("Can't assign, left value must be variable.");

					ParseNode* set = ParseNode::Get();
					switch(left->var->subtype)
					{
					case ParseVar::LOCAL:
						set->op = SET_LOCAL;
						break;
					case ParseVar::GLOBAL:
						set->op = SET_GLOBAL;
						break;
					case ParseVar::ARG:
						set->op = SET_ARG;
						break;
					default:
						assert(0);
						break;
					}
					set->var = left->var;
					set->type = left->type;
					set->ref = NO_REF;

					if(si.op == NOP)
					{
						// assign
						if(!TryCast(right, left->type))
							t.Throw("Can't assign '%s' to variable '%s %s'.", var_info[right->type].name, var_info[set->type].name, set->var->name.c_str());
						set->push(right);
					}
					else
					{
						// compound assign
						VAR_TYPE cast, result;
						if(!CanOp((SYMBOL)si.op, left->type, right->type, cast, result))
							t.Throw("Invalid types '%s' and '%s' for operation '%s'.", var_info[left->type].name, var_info[right->type].name, si.name);

						Cast(left, cast);
						Cast(right, cast);

						ParseNode* op = ParseNode::Get();
						op->op = (Op)symbols[si.op].op;
						op->type = result;
						op->ref = NO_REF;
						op->push(left);
						op->push(right);

						Cast(op, set->var->type);

						set->push(op);
					}

					stack2.push_back(set);
				}
				else
				{
					VAR_TYPE cast, result;
					if(!CanOp(si.symbol, left->type, right->type, cast, result))
						t.Throw("Invalid types '%s' and '%s' for operation '%s'.", var_info[left->type].name, var_info[right->type].name, si.name);

					Cast(left, cast);
					Cast(right, cast);

					ParseNode* op = ParseNode::Get();
					op->type = result;
					op->ref = NO_REF;

					if(!TryConstExpr(left, right, op, si.symbol))
					{
						op->op = (Op)si.op;
						op->push(left);
						op->push(right);
					}

					stack2.push_back(op);
				}
			}
		}
		else
			stack2.push_back(sn.node);
	}

	if(stack2.size() != 1u)
		t.Throw("Invalid operations.");

	return stack2.back();
}

ParseNode* ParseVarDecl(VAR_TYPE type, string* _name)
{
	// var_name
	const string& name = (_name ? *_name : t.MustGetItem());
	if(!_name)
		CheckFindItem(name);

	ParseVar* var = ParseVar::Get();
	var->name = name;
	var->type = type;
	var->index = current_block->var_offset;
	var->subtype = (current_function == nullptr ? ParseVar::GLOBAL : ParseVar::LOCAL);
	current_block->vars.push_back(var);
	current_block->var_offset++;
	if(!_name)
		t.Next();

	// [=]
	if(!t.IsSymbol('='))
		return nullptr;
	t.Next();

	// expr<,;>
	ParseNode* expr = ParseExpr(',', ';');
	if(!TryCast(expr, type))
		t.Throw("Can't assign type '%s' to variable '%s %s'.", var_info[expr->type].name, var->name.c_str(), var_info[type].name);

	ParseNode* node = ParseNode::Get();
	switch(var->subtype)
	{
	case ParseVar::LOCAL:
		node->op = SET_LOCAL;
		break;
	case ParseVar::GLOBAL:
		node->op = SET_GLOBAL;
		break;
	case ParseVar::ARG:
		node->op = SET_ARG;
		break;
	default:
		assert(0);
		break;
	}
	node->type = var->type;
	node->var = var;
	node->ref = NO_REF;
	node->push(expr);
	return node;
}

ParseNode* ParseCond()
{
	t.AssertSymbol('(');
	t.Next();
	ParseNode* cond = ParseExpr(')');
	t.AssertSymbol(')');
	if(!TryCast(cond, V_BOOL))
		t.Throw("Condition expression with '%s' type.", var_info[cond->type].name);
	t.Next();
	return cond;
}

ParseNode* ParseVarTypeDecl(VAR_TYPE* _type = nullptr, string* _name = nullptr)
{
	// var_type
	VAR_TYPE type;
	if(_type)
		type = *_type;
	else
		type = (VAR_TYPE)t.GetKeywordId();
	if(type == V_VOID)
		t.Throw("Can't declare void variable.");
	if(!_type)
		t.Next();

	// var_decl(s)
	vector<ParseNode*> nodes;
	do
	{
		ParseNode* decl = ParseVarDecl(type, _name);
		_name = nullptr;
		if(decl)
			nodes.push_back(decl);
		if(t.IsSymbol(';'))
			break;
		t.AssertSymbol(',');
		t.Next();
	} while(true);

	// node
	if(nodes.empty())
		return nullptr;
	else if(nodes.size() == 1u)
		return nodes.back();
	else
	{
		ParseNode* node = ParseNode::Get();
		node->pseudo_op = GROUP;
		node->type = V_VOID;
		node->ref = NO_REF;
		node->childs = nodes;
		return node;
	}
}

// can return null
ParseNode* ParseLine()
{
	if(t.IsSymbol(';'))
	{
		// empty statement
		t.Next();
		return nullptr;
	}
	else if(t.IsKeywordGroup(G_KEYWORD))
	{
		KEYWORD keyword = (KEYWORD)t.GetKeywordId(G_KEYWORD);
		switch(keyword)
		{
		case K_IF:
			{
				t.Next();
				ParseNode* if_expr = ParseCond();

				ParseNode* if_op = ParseNode::Get();
				if_op->pseudo_op = IF;
				if_op->type = V_VOID;
				if_op->ref = NO_REF;
				if_op->push(if_expr);
				if_op->push(ParseLineOrBlock());

				// else
				if(t.IsKeyword(K_ELSE, G_KEYWORD))
				{
					t.Next();
					if_op->push(ParseLineOrBlock());
				}
				else
					if_op->push(nullptr);

				return if_op;
			}
		case K_DO:
			{
				t.Next();
				++breakable_block;
				ParseNode* block = ParseLineOrBlock();
				--breakable_block;
				t.AssertKeyword(K_WHILE, G_KEYWORD);
				t.Next();
				ParseNode* cond = ParseCond();
				t.AssertSymbol(';');
				t.Next();
				ParseNode* do_whil = ParseNode::Get();
				do_whil->pseudo_op = DO_WHILE;
				do_whil->type = V_VOID;
				do_whil->value = DO_WHILE_NORMAL;
				do_whil->ref = NO_REF;
				do_whil->push(block);
				do_whil->push(cond);
				return do_whil;
			}
		case K_WHILE:
			{
				t.Next();
				ParseNode* cond = ParseCond();
				++breakable_block;
				ParseNode* block = ParseLineOrBlock();
				--breakable_block;
				ParseNode* whil = ParseNode::Get();
				whil->pseudo_op = WHILE;
				whil->type = V_VOID;
				whil->ref = NO_REF;
				whil->push(cond);
				whil->push(block);
				return whil;
			}
		case K_FOR:
			{
				t.Next();
				t.AssertSymbol('(');
				t.Next();

				ParseNode* for1, *for2, *for3;
				Block* new_block = Block::Get();
				Block* old_block = current_block;
				new_block->parent = old_block;
				new_block->var_offset = old_block->var_offset;
				old_block->childs.push_back(new_block);
				current_block = new_block;

				if(t.IsKeywordGroup(G_VAR))
					for1 = ParseVarTypeDecl();
				else if(t.IsSymbol(';'))
					for1 = nullptr;
				else
					for1 = ParseExpr(';');
				t.Next();
				if(t.IsSymbol(';'))
					for2 = nullptr;
				else
					for2 = ParseExpr(';');
				t.Next();
				if(t.IsSymbol(')'))
					for3 = nullptr;
				else
					for3 = ParseExpr(')');
				t.Next();

				if(for2 && !TryCast(for2, V_BOOL))
					t.Throw("Condition expression with '%s' type.", var_info[for2->type].name);

				ParseNode* fo = ParseNode::Get();
				fo->pseudo_op = FOR;
				fo->type = V_VOID;
				fo->ref = NO_REF;
				fo->push(for1);
				fo->push(for2);
				fo->push(for3);
				++breakable_block;
				fo->push(ParseLineOrBlock());
				--breakable_block;
				current_block = old_block;
				return fo;
			}
		case K_BREAK:
			{
				if(breakable_block == 0)
					t.Unexpected("Not in breakable block.");
				t.Next();
				t.AssertSymbol(';');
				t.Next();
				ParseNode* br = ParseNode::Get();
				br->pseudo_op = BREAK;
				br->type = V_VOID;
				br->ref = NO_REF;
				return br;
			}
		case K_RETURN:
			{
				if(!current_function)
					t.Unexpected("Not inside function.");
				ParseNode* ret = ParseNode::Get();
				ret->pseudo_op = RETURN;
				ret->type = V_VOID;
				ret->ref = NO_REF;
				t.Next();
				VAR_TYPE ret_type;
				if(!t.IsSymbol(';'))
				{
					ret->push(ParseExpr(';'));
					ret_type = ret->childs[0]->type;
					t.AssertSymbol(';');
				}
				else
					ret_type = V_VOID;
				if(ret_type != current_function->result)
					t.Throw("Invalid return type '%s', function '%s' require '%s' type.",
						var_info[ret_type].name, current_function->name.c_str(), var_info[current_function->result].name);
				t.Next();
				return ret;
			}
		default:
			t.Unexpected();
		}
	}
	else if(t.IsKeywordGroup(G_VAR))
	{
		// is this function or var declaration
		VAR_TYPE type = (VAR_TYPE)t.GetKeywordId(G_VAR);
		t.Next();
		LocalString str = t.MustGetItem();
		CheckFindItem(str.get_ref());
		t.Next();
		if(t.IsSymbol('('))
		{
			// function
			if(current_block != main_block)
				t.Throw("Function can't be declared inside block.");
			t.Next();
			ParseFunction* f = new ParseFunction;
			f->name = str;
			f->index = ufuncs.size();
			f->result = type;
			f->required_args = 0;
			current_function = f;

			// args
			bool prev_arg_def = false;
			if(!t.IsSymbol(')'))
			{
				while(true)
				{
					VAR_TYPE type = (VAR_TYPE)t.MustGetKeywordId(G_VAR);
					t.Next();
					const string& id = t.MustGetItem();
					CheckFindItem(id);
					ParseVar* arg = ParseVar::Get();
					arg->name = id;
					arg->type = type;
					arg->subtype = ParseVar::ARG;
					arg->index = current_function->args.size();
					current_function->args.push_back(arg);
					t.Next();
					if(t.IsSymbol('='))
					{
						prev_arg_def = true;
						t.Next();
						ParseNode* item = ParseConstItem();
						if(!TryCast(item, type))
							t.Throw("Invalid default value of type '%s', required '%s'.", var_info[item->type].name, var_info[type].name);
						switch(item->op)
						{
						case PUSH_BOOL:
							f->arg_infos.push_back(ArgInfo(item->bvalue));
							break;
						case PUSH_INT:
							f->arg_infos.push_back(ArgInfo(item->value));
							break;
						case PUSH_FLOAT:
							f->arg_infos.push_back(ArgInfo(item->fvalue));
							break;
						case PUSH_STRING:
							f->arg_infos.push_back(ArgInfo(V_STRING, item->value));
							break;
						default:
							assert(0);
							break;
						}
					}
					else
					{
						if(prev_arg_def)
							t.Throw("Missing default value for argument '%s'.", arg->name.c_str());
						f->arg_infos.push_back(ArgInfo(type));
						f->required_args++;
					}
					if(t.IsSymbol(')'))
						break;
					t.AssertSymbol(',');
					t.Next();
				}
			}
			t.Next();
			
			// block
			f->node = ParseBlock(f);
			current_function = nullptr;
			ufuncs.push_back(f);
			return nullptr;
		}
		else
		{
			// var
			ParseNode* node = ParseVarTypeDecl(&type, str.get_ptr());
			t.Next();
			return node;
		}
	}

	ParseNode* node = ParseExpr(';');

	// ;
	t.AssertSymbol(';');
	t.Next();

	return node;
}

// can return null
ParseNode* ParseBlock(ParseFunction* f)
{
	t.AssertSymbol('{');

	// block
	Block* new_block = Block::Get();
	Block* old_block = current_block;
	if(f)
	{
		new_block->parent = nullptr;
		new_block->var_offset = 0;
		f->block = new_block;
	}
	else
	{
		new_block->parent = old_block;
		new_block->var_offset = old_block->var_offset;
		old_block->childs.push_back(new_block);
	}
	current_block = new_block;

	t.Next();
	vector<ParseNode*> nodes;
	while(true)
	{
		if(t.IsSymbol('}'))
			break;
		ParseNode* node = ParseLineOrBlock();
		if(node)
			nodes.push_back(node);
	}
	t.Next();

	current_block = old_block;

	if(nodes.empty())
		return nullptr;
	else if(nodes.size() == 1u)
		return nodes.front();
	else
	{
		ParseNode* node = ParseNode::Get();
		node->pseudo_op = GROUP;
		node->type = V_VOID;
		node->ref = NO_REF;
		node->childs = nodes;
		return node;
	}
}

// can return null
ParseNode* ParseLineOrBlock()
{
	if(t.IsSymbol('{'))
		return ParseBlock();
	else
		return ParseLine();
}

ParseNode* ParseCode()
{
	breakable_block = 0;
	main_block = Block::Get();
	main_block->parent = nullptr;
	main_block->var_offset = 0u;
	current_block = main_block;
	current_function = nullptr;

	ParseNode* node = ParseNode::Get();
	node->pseudo_op = GROUP;
	node->type = V_VOID;
	node->ref = NO_REF;

	t.Next();
	while(!t.IsEof())
	{
		ParseNode* child = ParseLineOrBlock();
		if(child)
			node->push(child);
	}

	return node;
}

// childs can be nullptr only for if/while
ParseNode* OptimizeTree(ParseNode* node)
{
	assert(node);

	if(node->pseudo_op == IF)
	{
		ParseNode*& cond = node->childs[0];
		OptimizeTree(cond);
		ParseNode*& trueb = node->childs[1];
		if(trueb)
			OptimizeTree(trueb);
		ParseNode*& falseb = node->childs[2];
		if(falseb)
			OptimizeTree(falseb);
		if(cond->op == PUSH_BOOL)
		{
			// const expr
			ParseNode* result;
			if(cond->bvalue)
			{
				// always true (prevent Free node from Freeing trueb)
				result = trueb;
				trueb = nullptr;
			}
			else
			{
				// always false (prevent Free node from Freeing falseb)
				result = falseb;
				falseb = nullptr;
			}
			node->Free();
			return result;
		}
		else
		{
			// not const expr
			if(trueb == nullptr && falseb == nullptr)
			{
				// both code blocks are empty
				node->Free();
				return nullptr;
			}
			else if(trueb == nullptr)
			{
				// if block is empty, negate if and drop else
				ParseNode* not = ParseNode::Get();
				not->op = NOT;
				not->type = V_BOOL;
				not->ref = NO_REF;
				not->push(cond);
				node->childs[0] = not;
				node->childs[1] = node->childs[2];
				node->childs.pop_back();
			}
			else if(falseb == nullptr)
			{
				// else block is empty
				node->childs.pop_back();
			}
		}
	}
	else if(node->pseudo_op == DO_WHILE)
	{
		ParseNode*& block = node->childs[0];
		ParseNode*& cond = node->childs[1];
		if(block)
			OptimizeTree(block);
		OptimizeTree(cond);
		if(cond->pseudo_op == PUSH_BOOL)
		{
			// const cond
			if(cond->bvalue)
			{
				// do {...} while(true);
				node->value = DO_WHILE_INF;
			}
			else
			{
				// do {...} while(false);
				node->value = DO_WHILE_ONCE;
			}
			cond->Free();
			cond = nullptr;
		}
	}
	else if(node->pseudo_op == FOR)
	{
		ParseNode*& for1 = node->childs[0];
		ParseNode*& for2 = node->childs[1];
		ParseNode*& for3 = node->childs[2];
		ParseNode*& block = node->childs[3];
		if(for1)
			OptimizeTree(for1);
		if(for2)
			OptimizeTree(for2);
		if(for3)
			OptimizeTree(for3);
		if(block)
			OptimizeTree(block);
		if(for1)
		{
			if(for2 && for2->op == PUSH_BOOL)
			{
				// const condition
				if(for2->bvalue)
				{
					// infinite loop (set for2 to null)
					for2 = nullptr;
				}
				else
				{
					// loop zero times, only initialization block is used
					ParseNode* result = for1;
					for1 = nullptr;
					node->Free();
					return result;
				}
			}
		}
		else
		{
			// no initialization block
			if(for2 && for2->op == PUSH_BOOL)
			{
				// const condition
				if(for2->bvalue)
				{
					// infinite loop (set for2 to null)
					for2 = nullptr;
				}
				else
				{
					// loop zero times, no initialization block required so ignore this node
					node->Free();
					return nullptr;
				}
			}
		}
	}
	else if(node->pseudo_op == WHILE)
	{
		ParseNode*& cond = node->childs[0];
		OptimizeTree(cond);
		ParseNode*& block = node->childs[1];
		if(block)
			OptimizeTree(block);
		if(cond->pseudo_op == PUSH_BOOL)
		{
			// const cond
			if(cond->bvalue)
			{
				// while(true)
				cond->Free();
				cond = nullptr;
			}
			else
			{
				// while(false) - ignore
				node->Free();
				return nullptr;
			}
		}
	}
	else
	{
		LoopAndRemove(node->childs, [](ParseNode*& node) {
			node = OptimizeTree(node);
			return !node;
		});
	}

	return node;
}

void ToCode(vector<int>& code, ParseNode* node, vector<uint>* break_pos)
{
	if(node->pseudo_op == IF)
	{
		// if condition
		assert(node->childs.size() == 2u || node->childs.size() == 3u);
		ToCode(code, node->childs[0], break_pos);
		code.push_back(FJMP);
		uint tjmp_pos = code.size();
		code.push_back(0);
		if(node->childs.size() == 3u)
		{
			/*
			if expr
			fjmp else_block
			if_block: if code
			jmp end
			else_block: else code
			end:
			*/
			if(node->childs[1])
			{
				ToCode(code, node->childs[1], break_pos);
				if(node->childs[1]->type != V_VOID)
					code.push_back(POP);
			}
			code.push_back(JMP);
			uint jmp_pos = code.size();
			code.push_back(0);
			uint else_start = code.size();
			if(node->childs[2])
			{
				ToCode(code, node->childs[2], break_pos);
				if(node->childs[2]->type != V_VOID)
					code.push_back(POP);
			}
			uint end_start = code.size();
			code[tjmp_pos] = else_start;
			code[jmp_pos] = end_start;
		}
		else
		{
			/*
			if expr
			fjmp end
			if_code
			end:
			*/
			if(node->childs[1])
			{
				ToCode(code, node->childs[1], break_pos);
				if(node->childs[1]->type != V_VOID)
					code.push_back(POP);
			}
			uint end_start = code.size();
			code[tjmp_pos] = end_start;
		}
		return;
	}
	else if(node->pseudo_op == DO_WHILE)
	{
		assert(node->childs.size() == 2u);
		uint start = code.size();
		vector<uint> wh_break_pos;
		ParseNode* block = node->childs[0];
		ParseNode* cond = node->childs[1];
		if(block)
			ToCode(code, block, &wh_break_pos);
		if(node->value == DO_WHILE_NORMAL)
		{
			/*
			start:
				block
				cond
				tjmp start
			end:
			*/
			ToCode(code, cond, &wh_break_pos);
			code.push_back(TJMP);
			code.push_back(start);
		}
		else if(node->value == DO_WHILE_INF)
		{
			/*
			start:
				block
				jmp start
			end:
			*/
			code.push_back(JMP);
			code.push_back(start);
		}
		uint end_pos = code.size();
		for(uint p : wh_break_pos)
			code[p] = end_pos;
		return;
	}
	else if(node->pseudo_op == WHILE)
	{
		assert(node->childs.size() == 2u);
		uint start = code.size();
		vector<uint> wh_break_pos;
		ParseNode* cond = node->childs[0];
		ParseNode* block = node->childs[1];
		if(cond)
		{
			/*
			start:
				cond
				fjmp end
				block
				jmp start
			end:
			*/
			ToCode(code, cond, &wh_break_pos);
			code.push_back(FJMP);
			uint end_jmp = code.size();
			code.push_back(0);
			if(block)
				ToCode(code, block, &wh_break_pos);
			code.push_back(JMP);
			code.push_back(start);
			code[end_jmp] = code.size();
		}
		else
		{
			/*
			start:
				block
				jmp start
			*/
			if(block)
				ToCode(code, block, &wh_break_pos);
			code.push_back(JMP);
			code.push_back(start);
		}
		uint end_pos = code.size();
		for(uint p : wh_break_pos)
			code[p] = end_pos;
		return;
	}
	else if(node->pseudo_op == FOR)
	{
		/*
		for1
		start:
			[for2
			fjmp end]
			block
			for3
			jmp start
		end:
		*/
		assert(node->childs.size() == 4u);
		ParseNode* for1 = node->childs[0];
		ParseNode* for2 = node->childs[1];
		ParseNode* for3 = node->childs[2];
		ParseNode* block = node->childs[3];
		vector<uint> wh_break_pos;
		if(for1)
		{
			ToCode(code, for1, &wh_break_pos);
			if(for1->type != V_VOID)
				code.push_back(POP);
		}
		uint start = code.size();
		uint fjmp_pos = 0;
		if(for2)
		{
			ToCode(code, for2, &wh_break_pos);
			code.push_back(FJMP);
			fjmp_pos = code.size();
			code.push_back(0);
		}
		if(block)
		{
			ToCode(code, block, &wh_break_pos);
			if(block->type != V_VOID)
				code.push_back(POP);
		}
		if(for3)
		{
			ToCode(code, for3, &wh_break_pos);
			if(for3->type != V_VOID)
				code.push_back(POP);
		}
		code.push_back(JMP);
		code.push_back(start);
		uint end_pos = code.size();
		if(fjmp_pos != 0)
			code[fjmp_pos] = end_pos;
		for(uint p : wh_break_pos)
			code[p] = end_pos;
		return;
	}
	else if(node->pseudo_op == GROUP)
	{
		for(ParseNode* n : node->childs)
		{
			ToCode(code, n, break_pos);
			if(n->type != V_VOID)
				code.push_back(POP);
		}
		return;
	}

	for(ParseNode* n : node->childs)
		ToCode(code, n, break_pos);
	
	switch(node->op)
	{
	case PUSH_INT:
	case PUSH_STRING:
	case CALL:
	case CALLU:
	case CAST:
		code.push_back(node->op);
		code.push_back(node->value);
		break;
	case PUSH_LOCAL:
	case PUSH_LOCAL_REF:
	case PUSH_GLOBAL:
	case PUSH_GLOBAL_REF:
	case PUSH_ARG:
	case PUSH_ARG_REF:
	case SET_LOCAL:
	case SET_GLOBAL:
	case SET_ARG:
		code.push_back(node->op);
		code.push_back(node->var->index);
		break;
	case PUSH_FLOAT:
		code.push_back(node->op);
		code.push_back(*(int*)&node->fvalue);
		break;
	case NEG:
	case ADD:
	case SUB:
	case MUL:
	case MOD:
	case DIV:
	case EQ:
	case NOT_EQ:
	case GR:
	case GR_EQ:
	case LE:
	case LE_EQ:
	case AND:
	case OR:
	case NOT:
	case PRE_INC:
	case PRE_DEC:
	case POST_INC:
	case POST_DEC:
	case DEREF:
		code.push_back(node->op);
		break;
	case PUSH_BOOL:
		code.push_back(node->bvalue ? PUSH_TRUE : PUSH_FALSE);
		break;
	case BREAK:
		assert(break_pos);
		code.push_back(JMP);
		break_pos->push_back(code.size());
		code.push_back(0);
		break;
	case RETURN:
		code.push_back(RET);
		break;
	default:
		assert(0);
		break;
	}
}

bool VerifyNodeReturnValue(ParseNode* node)
{
	switch(node->pseudo_op)
	{
	case GROUP:
		for(vector<ParseNode*>::reverse_iterator it = node->childs.rbegin(), end = node->childs.rend(); it != end; ++it)
		{
			if(VerifyNodeReturnValue(*it))
				return true;
		}
		return false;
	case IF:
		return (node->childs.size() == 3u && node->childs[1] && node->childs[2]
			&& VerifyNodeReturnValue(node->childs[1]) && VerifyNodeReturnValue(node->childs[2]));
	case RETURN:
		return true;
	default:
		return false;
	}
}

void VerifyFunctionReturnValue(ParseFunction* f)
{
	if(f->result == V_VOID)
		return;
		
	if(f->node)
	{
		for(vector<ParseNode*>::reverse_iterator it = f->node->childs.rbegin(), end = f->node->childs.rend(); it != end; ++it)
		{
			if(VerifyNodeReturnValue(*it))
				return;
		}
	}

	t.Throw("Function '%s' not always return value.", f->name.c_str());
}

bool Parse(ParseContext& ctx)
{
	try
	{
		optimize = ctx.optimize;
		t.FromString(ctx.input);

		// parse
		ParseNode* node = ParseCode();

		// optimize
		if(optimize)
		{
			for(ParseFunction* ufunc : ufuncs)
				OptimizeTree(ufunc->node);
			OptimizeTree(node);
		}

		// verify
		for(ParseFunction* ufunc : ufuncs)
			VerifyFunctionReturnValue(ufunc);
		
		// codify
		for(ParseFunction* ufunc : ufuncs)
		{
			ufunc->pos = ctx.code.size();
			ufunc->locals = ufunc->block->GetMaxVars();
			ToCode(ctx.code, ufunc->node, nullptr);
			ctx.code.push_back(RET);
		}
		ctx.entry_point = ctx.code.size();
		ToCode(ctx.code, node, nullptr);
		ctx.code.push_back(RET);

		ctx.strs = strs;
		ctx.ufuncs.resize(ufuncs.size());
		for(uint i = 0; i < ufuncs.size(); ++i)
		{
			ParseFunction& f = *ufuncs[i];
			UserFunction& uf = ctx.ufuncs[i];
			uf.pos = f.pos;
			uf.locals = f.locals;
			uf.result = f.result;
#ifdef _DEBUG
			for(ParseVar* arg : f.args)
				uf.args.push_back(arg->type);
#else
			uf.args = f.args.size();
#endif
		}
		ctx.globals = main_block->GetMaxVars();

		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		cout << Format("Parse error: %s", e.ToString());
		return false;
	}
}

void InitializeParser()
{
	// register var types
	for(int i = 0; i < V_MAX; ++i)
	{
		VarInfo& vi = var_info[i];
		if(vi.reg)
			t.AddKeyword(vi.name, i, G_VAR);
	}

	// register keywords
	t.AddKeywords(G_KEYWORD, {
		{"if", K_IF},
		{"else", K_ELSE},
		{"do", K_DO},
		{"while", K_WHILE},
		{"for", K_FOR},
		{"break", K_BREAK},
		{"return", K_RETURN}
	});

	// const
	t.AddKeywords(G_CONST, {
		{"true", C_TRUE},
		{"false", C_FALSE}
	});
}

void CleanupParser()
{
	Str::Free(strs);
	DeleteElements(ufuncs);
}

struct OpInfo
{
	Op op;
	cstring name;
	VAR_TYPE arg1;
};

OpInfo ops[MAX_OP] = {
	PUSH_TRUE, "push_true", V_VOID,
	PUSH_FALSE, "push_false", V_VOID,
	PUSH_INT, "push_int", V_INT,
	PUSH_FLOAT, "push_float", V_FLOAT,
	PUSH_STRING, "push_string", V_INT,
	PUSH_LOCAL, "push_local", V_INT,
	PUSH_LOCAL_REF, "push_local_ref", V_INT,
	PUSH_GLOBAL, "push_global", V_INT,
	PUSH_GLOBAL_REF, "push_global_ref", V_INT,
	PUSH_ARG, "push_arg", V_INT,
	PUSH_ARG_REF, "push_arg_ref", V_INT,
	POP, "pop", V_VOID,
	SET_LOCAL, "set_local", V_INT,
	SET_GLOBAL, "set_global", V_INT,
	SET_ARG, "set_arg", V_INT,
	CAST, "cast", V_INT,
	NEG, "neg", V_VOID,
	ADD, "add", V_VOID,
	SUB, "sub", V_VOID,
	MUL, "mul", V_VOID,
	DIV, "div", V_VOID,
	MOD, "mod", V_VOID,
	PRE_INC, "pre_inc", V_VOID,
	PRE_DEC, "pre_dec", V_VOID,
	POST_INC, "post_inc", V_VOID,
	POST_DEC, "post_dec", V_VOID,
	DEREF, "deref", V_VOID,
	EQ, "eq", V_VOID,
	NOT_EQ, "not_eq", V_VOID,
	GR, "gr", V_VOID,
	GR_EQ, "gr_eq", V_VOID,
	LE, "le", V_VOID,
	LE_EQ, "le_eq", V_VOID,
	AND, "and", V_VOID,
	OR, "or", V_VOID,
	NOT, "not", V_VOID,
	JMP, "jmp", V_INT,
	TJMP, "tjmp", V_INT,
	FJMP, "fjmp", V_INT,
	CALL, "call", V_INT,
	CALLU, "callu", V_INT,
	RET, "ret", V_VOID
};

template<typename To, typename From>
inline To union_cast(const From& f)
{
	union
	{
		To to;
		From from;
	} a;

	a.from = f;
	return a.to;
}

void Decompile(ParseContext& ctx)
{
	int* c = ctx.code.data();
	int* end = c + ctx.code.size();

	cout << "DECOMPILE:\n";
	while(c != end)
	{
		Op op = (Op)*c++;
		if(op >= MAX_OP)
			cout << "MISSING (" << op << ")\n";
		else
		{
			OpInfo& opi = ops[op];
			switch(opi.arg1)
			{
			case V_VOID:
				cout << Format("[%d] %s\n", (int)op, opi.name);
				break;
			case V_INT:
				{
					int value = *c++;
					cout << Format("[%d %d] %s %d\n", (int)op, value, opi.name, value);
				}
				break;
			case V_FLOAT:
				{
					int val = *c++;
					float value = union_cast<float>(val);
					cout << Format("[%d %d] %s %g\n", (int)op, val, opi.name, value);
				}
				break;
			}
		}
	}
	cout << "\n";
}
