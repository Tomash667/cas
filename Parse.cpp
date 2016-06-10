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
	K_WHILE
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
	F_FUNC
};

enum PseudoOp
{
	GROUP = MAX_OP,
	PUSH_BOOL,
	NOP,
	IF,
	PRE_CALL,
	WHILE
};

struct ParseVar : ObjectPoolProxy<ParseVar>
{
	string name;
	int index;
	VAR_TYPE type;
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

	inline void push(ParseNode* p) { childs.push_back(p); }
	inline void OnFree() { SafeFree(childs); }
};

union Found
{
	ParseVar* var;
	Function* func;
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


static Tokenizer t;
static vector<string> strs;
static Block* main_block;
static Block* current_block;
static bool optimize;


void ParseArgs(vector<ParseNode*>& nodes);
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

	ParseVar* var = current_block->FindVar(id);
	if(var)
	{
		found.var = var;
		return F_VAR;
	}

	return F_NONE;
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
	if(type == V_VOID || node->type == type)
		return;

	// can const cast?
	if(TryConstCast(node, type))
		return;

	// normal cast
	ParseNode* cast = ParseNode::Get();
	cast->op = CAST;
	cast->value = type;
	cast->type = type;
	cast->push(node);
	node = cast;
}

// used in var assignment, passing argument to function
bool TryCast(ParseNode*& node, VAR_TYPE type)
{
	// no cast required?
	if(node->type == type)
		return true;

	// no implicit cast from string to bool/int/float
	if(node->type == V_STRING && (type == V_BOOL || type == V_INT || type == V_FLOAT))
		return false;

	Cast(node, type);
	return true;
}

void VerifyFunctionCall(ParseNode* node, Function* f)
{
	if(node->childs.size() != f->args.size())
		t.Throw("Function %s with %d arguments not found, function have %d arguments.", f->name, node->childs.size(), f->args.size());
	for(uint i = 0; i < node->childs.size(); ++i)
	{
		if(!TryCast(node->childs[i], f->args[i]))
			t.Throw("Function %s takes %s for argument %d, found %s.", f->name, var_name[f->args[i]], i + 1, var_name[node->childs[i]->type]);
	}
}

ParseNode* ParseItem()
{
	if(t.IsInt())
	{
		// int
		int val = t.GetInt();
		ParseNode* node = ParseNode::Get();
		node->op = PUSH_INT;
		node->type = V_INT;
		node->value = val;
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
		t.Next();
		return node;
	}
	else if(t.IsItem())
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
				node->op = PUSH_VAR;
				node->type = var->type;
				node->var = var;
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

				t.Next();
				ParseArgs(node->childs);
				VerifyFunctionCall(node, f);

				return node;
			}
		default:
			assert(0);
		case F_NONE:
			t.Unexpected();
		}
	}
	else if(t.IsString())
	{
		// string
		int index = strs.size();
		strs.push_back(t.GetString());
		ParseNode* node = ParseNode::Get();
		node->op = PUSH_STRING;
		node->value = index;
		node->type = V_STRING;
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
		return node;
	}
	else
		t.Unexpected();
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
	S_INVALID,
	S_MAX
};

// http://en.cppreference.com/w/cpp/language/operator_precedence
struct SymbolInfo
{
	SYMBOL symbol;
	cstring name;
	int priority;
	bool left_associativity;
	int args, op;
	bool assign;
};

SymbolInfo symbols[S_MAX] = {
	S_ADD, "add", 5, true, 2, ADD, false,
	S_SUB, "subtract", 5, true, 2, SUB, false,
	S_MUL, "multiply", 6, true, 2, MUL, false,
	S_DIV, "divide", 6, true, 2, DIV, false,
	S_MOD, "modulo", 6, true, 2, MOD, false,
	S_PLUS, "unary plus", 7, false, 1, NOP, false,
	S_MINUS, "unary minus", 7, false, 1, NEG, false,
	S_LEFT_PAR, "left parenthesis", -1, true, 0, NOP, false,
	S_EQUAL, "equal", 3, true, 2, EQ, false,
	S_NOT_EQUAL, "not equal", 3, true, 2, NOT_EQ, false,
	S_GREATER, "greater", 4, true, 2, GR, false,
	S_GREATER_EQUAL, "greater equal", 4, true, 2, GR_EQ, false,
	S_LESS, "less", 4, true, 2, LE, false,
	S_LESS_EQUAL, "less equal", 4, true, 2, LE_EQ, false,
	S_AND, "and", 2, true, 2, AND, false,
	S_OR, "or", 1, true, 2, OR, false,
	S_NOT, "not", 7, false, 1, NOT, false,
	S_MEMBER_ACCESS, "member access", 7, true, 2, NOP, false,
	S_ASSIGN, "assign", 0, false, 2, NOP, true,
	S_ASSIGN_ADD, "assign add", 0, false, 2, S_ADD, true,
	S_ASSIGN_SUB, "assign subtract", 0, false, 2, S_SUB, true,
	S_ASSIGN_MUL, "assign multiply", 0, false, 2, S_MUL, true,
	S_ASSIGN_DIV, "assign divide", 0, false, 2, S_DIV, true,
	S_ASSIGN_MOD, "assign modulo", 0, false, 2, S_MOD, true,
	S_INVALID, "invalid", -1, true, 0, NOP, false
};

enum LEFT
{
	LEFT_NONE,
	LEFT_SYMBOL,
	LEFT_UNARY,
	LEFT_ITEM
};

SYMBOL CharToSymbol(char c)
{
	switch(c)
	{
	case '+':
		return S_ADD;
	case '-':
		return S_SUB;
	case '*':
		return S_MUL;
	case '/':
		return S_DIV;
	case '%':
		return S_MOD;
	default:
		assert(0);
		return S_INVALID;
	}
}

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

ParseNode* ParseExpr(char end, char end2)
{
	vector<SymbolOrNode> exit;
	vector<SYMBOL> stack;
	LEFT left = LEFT_NONE;
	int open_par = 0;

	while(true)
	{
		if(t.IsSymbol())
		{
			char c = t.GetSymbol();
			if(c == end || c == end2)
			{
				if(c != ')' || open_par == 0)
					break;
			}
			c = strchr2(c, "+-*/%()!=><&|.");
			if(c == 0)
				t.Unexpected();
			else if(c == '(')
			{
				if(left == LEFT_ITEM)
					t.Unexpected();
				stack.push_back(S_LEFT_PAR);
				t.Next();
				left = LEFT_NONE;
				++open_par;
			}
			else if(c == ')')
			{
				if(left != LEFT_ITEM)
					t.Unexpected();
				bool ok = false;
				while(!stack.empty())
				{
					SYMBOL s = stack.back();
					stack.pop_back();
					if(s == S_LEFT_PAR)
					{
						ok = true;
						break;
					}
					exit.push_back(s);
				}
				if(!ok)
					t.Unexpected();
				t.Next();
				left = LEFT_ITEM;
				--open_par;
			}
			else
			{
				SYMBOL symbol;
				switch(left)
				{
				case LEFT_NONE:
				case LEFT_SYMBOL:
				case LEFT_UNARY:
					{
						switch(c)
						{
						case '+':
							symbol = S_PLUS;
							break;
						case '-':
							symbol = S_MINUS;
							break;
						case '!':
							symbol = S_NOT;
							break;
						default:
							t.Unexpected();
						}
						left = LEFT_UNARY;
					}
					break;
				case LEFT_ITEM:
					switch(c)
					{
					case '+':
						if(t.PeekSymbol('='))
							symbol = S_ASSIGN_ADD;
						else
							symbol = S_ADD;
						break;
					case '-':
						if(t.PeekSymbol('='))
							symbol = S_ASSIGN_SUB;
						else
							symbol = S_SUB;
						break;
					case '*':
						if(t.PeekSymbol('='))
							symbol = S_ASSIGN_MUL;
						else
							symbol = S_MUL;
						break;
					case '/':
						if(t.PeekSymbol('='))
							symbol = S_ASSIGN_DIV;
						else
							symbol = S_DIV;
						break;
					case '%':
						if(t.PeekSymbol('='))
							symbol = S_ASSIGN_MOD;
						else
							symbol = S_MOD;
						break;
					case '!':
						if(!t.PeekSymbol('='))
							t.Unexpected();
						symbol = S_NOT_EQUAL;
						break;
					case '=':
						if(t.PeekSymbol('='))
							symbol = S_EQUAL;
						else
							symbol = S_ASSIGN;
						break;
					case '>':
						if(t.PeekSymbol('='))
							symbol = S_GREATER_EQUAL;
						else
							symbol = S_GREATER;
						break;
					case '<':
						if(t.PeekSymbol('='))
							symbol = S_LESS_EQUAL;
						else
							symbol = S_LESS;
						break;
					case '&':
						if(!t.PeekSymbol('&'))
							t.Unexpected();
						symbol = S_AND;
						break;
					case '|':
						if(!t.PeekSymbol('|'))
							t.Unexpected();
						symbol = S_OR;
						break;
					case '.':
						symbol = S_MEMBER_ACCESS;
						break;
					}
					left = LEFT_SYMBOL;
					break;
				}

				while(!stack.empty())
				{
					SYMBOL symbol2 = stack.back();
					SymbolInfo& s1 = symbols[symbol];
					SymbolInfo& s2 = symbols[symbol2];

					bool ok = false;
					if(s1.left_associativity)
						ok = (s1.priority <= s2.priority);
					else
						ok = (s1.priority < s2.priority);

					if(ok)
					{
						exit.push_back(symbol2);
						stack.pop_back();
					}
					else
						break;
				}

				stack.push_back(symbol);
				t.Next();

				if(symbol == S_MEMBER_ACCESS)
				{
					string* str = StringPool.Get();
					*str = t.MustGetItem();
					t.Next();
					ParseNode* node = ParseNode::Get();
					node->pseudo_op = PRE_CALL;
					node->type = V_VOID;
					node->str = str;
					ParseArgs(node->childs);
					exit.push_back(node);
					left = LEFT_ITEM;
				}
			}
		}
		else
		{
			if(left == LEFT_ITEM)
				t.Unexpected();
			exit.push_back(ParseItem());
			left = LEFT_ITEM;
		}
	}

	if(left == LEFT_SYMBOL || left == LEFT_UNARY)
		t.Unexpected();

	while(!stack.empty())
	{
		SYMBOL s = stack.back();
		stack.pop_back();
		if(s == S_LEFT_PAR)
			t.Throw("Missing closing parenthesis.");
		exit.push_back(s);
	}

	assert(open_par == 0);

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
				VAR_TYPE cast, result;
				if(!CanOp(sn.symbol, node->type, V_VOID, cast, result))
					t.Throw("Invalid type '%s' for operation '%s'.", var_name[node->type], si.name);
				Cast(node, cast);
				if(!TryConstExpr1(node, si.symbol) && si.op != NOP)
				{
					ParseNode* op = ParseNode::Get();
					op->op = (Op)si.op;
					op->type = result;
					op->push(node);
					node = op;
				}
				stack2.push_back(node);
			}
			else if(si.args == 2)
			{
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
						t.Throw("Missing function '%s' for type '%s'.", right->str->c_str(), var_name[left->type]);
					StringPool.Free(right->str);
					VerifyFunctionCall(right, func);
					ParseNode* node = ParseNode::Get();
					node->op = CALL;
					node->type = func->result;
					node->value = func->index;
					node->push(left);
					for(ParseNode* n : right->childs)
						node->push(n);
					right->Free();
					stack2.push_back(node);
				}
				else if(si.assign)
				{
					if(left->op != PUSH_VAR)
						t.Throw("Can't assign, left value must be variable.");

					ParseNode* set = ParseNode::Get();
					set->op = SET_VAR;
					set->var = left->var;
					set->type = left->type;

					if(si.op == NOP)
					{
						// assign
						if(!TryCast(right, left->type))
							t.Throw("Can't assign '%s' to variable '%s %s'.", var_name[right->type], var_name[set->type], set->var->name.c_str());
						set->push(right);
					}
					else
					{
						// compound assign
						VAR_TYPE cast, result;
						if(!CanOp((SYMBOL)si.op, left->type, right->type, cast, result))
							t.Throw("Invalid types '%s' and '%s' for operation '%s'.", var_name[left->type], var_name[right->type], si.name);

						Cast(left, cast);
						Cast(right, cast);

						ParseNode* op = ParseNode::Get();
						op->op = (Op)symbols[si.op].op;
						op->type = result;
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
						t.Throw("Invalid types '%s' and '%s' for operation '%s'.", var_name[left->type], var_name[right->type], si.name);

					Cast(left, cast);
					Cast(right, cast);

					ParseNode* op = ParseNode::Get();
					op->type = result;

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

ParseNode* ParseVarDecl(VAR_TYPE type)
{
	// var_name
	const string& name = t.MustGetItem();
	Found found;
	FOUND found_type = FindItem(name, found);
	if(found_type != F_NONE)
	{
		if(found_type == F_FUNC)
			t.Throw("Variable name already used as function.");
		else
			t.Throw("Variable already declared.");
	}

	ParseVar* var = ParseVar::Get();
	var->name = name;
	var->type = type;
	var->index = current_block->var_offset;
	current_block->vars.push_back(var);
	current_block->var_offset++;
	t.Next();

	// [=]
	if(!t.IsSymbol('='))
		return nullptr;
	t.Next();

	// expr<,;>
	ParseNode* expr = ParseExpr(',', ';');
	if(!TryCast(expr, type))
		t.Throw("Can't assign type '%s' to variable '%s %s'.", var_name[expr->type], var->name.c_str(), var_name[type]);

	ParseNode* node = ParseNode::Get();
	node->op = SET_VAR;
	node->type = var->type;
	node->var = var;
	node->push(expr);
	return node;
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
				t.AssertSymbol('(');
				t.Next();
				ParseNode* if_expr = ParseExpr(')');
				t.AssertSymbol(')');
				if(!TryCast(if_expr, V_BOOL))
					t.Throw("Condition expression with '%s' type.", var_name[if_expr->type]);
				t.Next();

				ParseNode* if_op = ParseNode::Get();
				if_op->pseudo_op = IF;
				if_op->type = V_VOID;
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
		case K_WHILE:
			{
				t.Next();
				t.AssertSymbol('(');
				t.Next();

				ParseNode* cond = ParseExpr(')');
				if(!TryCast(cond, V_BOOL))
					t.Throw("Condition expression with '%s' type.", var_name[cond->type]);

				t.AssertSymbol(')');
				t.Next();
				ParseNode* block = ParseLineOrBlock();
				ParseNode* whil = ParseNode::Get();
				whil->pseudo_op = WHILE;
				whil->type = V_VOID;
				whil->push(cond);
				whil->push(block);
				
				return whil;
			}
		default:
			t.Unexpected();
		}
		
	}
	else if(t.IsKeywordGroup(G_VAR))
	{
		// var_type
		VAR_TYPE type = (VAR_TYPE)t.GetKeywordId();
		if(type == V_VOID)
			t.Throw("Can't declare void variable.");
		t.Next();

		// var_decl(s)
		vector<ParseNode*> nodes;
		do
		{
			ParseNode* decl = ParseVarDecl(type);
			if(decl)
				nodes.push_back(decl);
			if(t.IsSymbol(';'))
				break;
			t.AssertSymbol(',');
			t.Next();
		} while(true);
		t.Next();

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
			node->childs = nodes;
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
ParseNode* ParseLineOrBlock()
{
	if(t.IsSymbol('{'))
	{
		// block
		Block* new_block = Block::Get();
		Block* old_block = current_block;
		new_block->parent = old_block;
		new_block->var_offset = old_block->var_offset;
		old_block->childs.push_back(new_block);
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
			node->childs = nodes;
			return node;
		}
	}
	else
		return ParseLine();
}

ParseNode* ParseCode()
{
	main_block = Block::Get();
	main_block->parent = nullptr;
	main_block->var_offset = 0u;
	current_block = main_block;

	ParseNode* node = ParseNode::Get();
	node->pseudo_op = GROUP;
	node->type = V_VOID;

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
				node->childs[0] = nullptr;
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

void ToCode(vector<int>& code, ParseNode* node)
{
	if(node->op == IF)
	{
		// if condition
		assert(node->childs.size() == 2u || node->childs.size() == 3u);
		ToCode(code, node->childs[0]);
		code.push_back(TJMP);
		uint tjmp_pos = code.size();
		code.push_back(0);
		if(node->childs.size() == 3u)
		{
			/*
			if expr
			tjmp else_block
			if_block: if code
			jmp end
			else_block: else code
			end:
			*/
			if(node->childs[1])
			{
				ToCode(code, node->childs[1]);
				if(node->childs[1]->type != V_VOID)
					code.push_back(POP);
			}
			code.push_back(JMP);
			uint jmp_pos = code.size();
			code.push_back(0);
			uint else_start = code.size();
			if(node->childs[2])
			{
				ToCode(code, node->childs[2]);
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
			tjmp end
			if_code
			end:
			*/
			if(node->childs[1])
			{
				ToCode(code, node->childs[1]);
				if(node->childs[1]->type != V_VOID)
					code.push_back(POP);
			}
			uint end_start = code.size();
			code[tjmp_pos] = end_start;
		}
		return;
	}
	else if(node->op == WHILE)
	{
		assert(node->childs.size() == 2u);
		uint start = code.size();
		ParseNode* cond = node->childs[0];
		ParseNode* block = node->childs[1];
		if(cond)
		{
			/*
			start:
				cond
				tjmp end
				block
				jmp start
			end:
			*/
			ToCode(code, cond);
			code.push_back(TJMP);
			uint end_jmp = code.size();
			code.push_back(0);
			if(block)
				ToCode(code, block);
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
				ToCode(code, block);
			code.push_back(JMP);
			code.push_back(start);
		}
		return;
	}
	else if(node->op == GROUP)
	{
		for(ParseNode* n : node->childs)
		{
			ToCode(code, n);
			if(n->type != V_VOID)
				code.push_back(POP);
		}
		return;
	}

	for(ParseNode* n : node->childs)
		ToCode(code, n);
	
	switch(node->op)
	{
	case PUSH_INT:
	case PUSH_STRING:
	case CALL:
	case CAST:
		code.push_back(node->op);
		code.push_back(node->value);
		break;
	case PUSH_VAR:
	case SET_VAR:
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
		code.push_back(node->op);
		break;
	case PUSH_BOOL:
		code.push_back(node->bvalue ? PUSH_TRUE : PUSH_FALSE);
		break;
	default:
		assert(0);
		break;
	}
}

bool Parse(ParseContext& ctx)
{
	try
	{
		optimize = ctx.optimize;
		t.FromString(ctx.input);

		// parse
		ParseNode* node = ParseCode();
		if(optimize)
			OptimizeTree(node);
		
		// codify
		ToCode(ctx.code, node);
		ctx.code.push_back(RET);

		ctx.strs = strs;
		ctx.vars = main_block->GetMaxVars();

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
		t.AddKeyword(var_name[i], i, G_VAR);

	// register keywords
	t.AddKeywords(G_KEYWORD, {
		{"if", K_IF},
		{"else", K_ELSE},
		{"while", K_WHILE}
	});

	// const
	t.AddKeywords(G_CONST, {
		{"true", C_TRUE},
		{"false", C_FALSE}
	});
}

void CleanupParser()
{
	strs.clear();
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
	PUSH_VAR, "push_var", V_INT,
	POP, "pop", V_VOID,
	SET_VAR, "set_var", V_INT,
	CAST, "cast", V_INT,
	NEG, "neg", V_VOID,
	ADD, "add", V_VOID,
	SUB, "sub", V_VOID,
	MUL, "mul", V_VOID,
	DIV, "div", V_VOID,
	MOD, "mod", V_VOID,
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
	CALL, "call", V_INT,
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

	cout << "DECMPILE:\n";
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
