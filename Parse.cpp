#include "Pch.h"
#include "Base.h"
#include "Op.h"
#include "Var.h"
#include "Function.h"
#include "Parse.h"

enum GROUP
{
	G_VAR
};

struct ParseVar : ObjectPoolProxy<ParseVar>
{
	string name;
	int index;
	VAR_TYPE type;
};

struct ParseNode : ObjectPoolProxy<ParseNode>
{
	Op op;
	union
	{
		int value;
		float fvalue;
	};
	VAR_TYPE type;
	vector<ParseNode*> childs;

	inline void push(ParseNode* p) { childs.push_back(p); }
};

Tokenizer t;
vector<ParseVar*> vars;
vector<int> pcode;
vector<string> strs;


ParseNode* ParseExpr(char end, char end2 = 0);


ParseVar* FindVar(const string& id)
{
	for(ParseVar* v : vars)
	{
		if(v->name == id)
			return v;
	}
	return nullptr;
}

bool TryConstCast(ParseNode* node, VAR_TYPE type)
{
	if((node->op == PUSH_INT || node->op == PUSH_FLOAT) && (type == V_INT || type == V_FLOAT))
	{
		// const cast
		if(node->op == PUSH_INT)
		{
			// int -> float
			node->op = PUSH_FLOAT;
			node->fvalue = (float)node->value;
		}
		else
		{
			// float -> int
			node->op = PUSH_INT;
			node->value = (int)node->fvalue;
		}
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

	// no implicit cast from string to int/float
	if(node->type == V_STRING && (type == V_INT || type == V_FLOAT))
		return false;

	Cast(node, type);
	return true;
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
		const string& item = t.GetItem();
		ParseVar* var = FindVar(item);
		Function* f;
		if(var)
		{
			ParseNode* node = ParseNode::Get();
			node->op = PUSH_VAR;
			node->type = var->type;
			node->value = var->index;
			t.Next();
			return node;
		}
		else if((f = FindFunction(item)))
		{
			// function
			t.Next();
			t.AssertSymbol('(');
			t.Next();

			// args
			ParseNode* node = ParseNode::Get();
			node->op = CALL;
			node->type = f->result;
			node->value = f->index;
			if(!t.IsSymbol(')'))
			{
				while(true)
				{
					node->push(ParseExpr(',', ')'));
					if(t.IsSymbol(')'))
						break;
					t.AssertSymbol(',');
					t.Next();
				}
			}
			t.Next();

			// verify
			if(node->childs.size() != f->args.size())
				t.Throw("Function %s with %d arguments not found, function have %d arguments.", f->name, node->childs.size(), f->args.size());
			for(uint i = 0; i < node->childs.size(); ++i)
			{
				if(!TryCast(node->childs[i], f->args[i]))
					t.Throw("Function %s takes %s for argument %d, found %s.", f->name, var_name[f->args[i]], i + 1, var_name[node->childs[i]->type]);
			}

			return node;
		}
		else
			t.Unexpected();
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
	else
		t.Unexpected();
}

char strchrs(cstring s, cstring chrs)
{
	assert(s && chrs);

	while(true)
	{
		char c = *s++;
		if(c == 0)
			return 0;
		cstring ch = chrs;
		while(true)
		{
			char c2 = *ch++;
			if(c2 == 0)
				break;
			if(c == c2)
				return c;
		}
	}
}

char strchr2(char c, cstring chrs)
{
	while(true)
	{
		char c2 = *chrs++;
		if(c2 == 0)
			return 0;
		if(c == c2)
			return c;
	}
}

enum SYMBOL
{
	S_ADD,
	S_SUB,
	S_MUL,
	S_DIV,
	S_PLUS,
	S_MINUS,
	S_LEFT_PAR,
	S_INVALID
};

// http://en.cppreference.com/w/cpp/language/operator_precedence
struct SymbolInfo
{
	SYMBOL symbol;
	cstring name;
	int priority;
	bool left_associativity;
	int args, op;
};

SymbolInfo symbols[] = {
	S_ADD, "add", 0, true, 2, ADD,
	S_SUB, "subtract", 0, true, 2, SUB,
	S_MUL, "multiply", 1, true, 2, MUL,
	S_DIV, "divide", 1, true, 2, DIV,
	S_PLUS, "unary plus", 2, false, 1, 0,
	S_MINUS, "unary minus", 2, false, 1, 0,
	S_LEFT_PAR, "left parenthesis", -1, true, 0, 0,
	S_INVALID, "invalid", -1, true, 0, 0,
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
	default:
		assert(0);
		return S_INVALID;
	}
}

bool CanOp(SYMBOL symbol, VAR_TYPE left, VAR_TYPE right, VAR_TYPE& cast, VAR_TYPE& result)
{
	switch(symbol)
	{
	case S_ADD:
		if(left == right)
		{
			// no cast required (int/float/string)
			cast = V_VOID;
			result = left;
			return true;
		}
		else if(left == V_STRING || right == V_STRING)
		{
			// cast int/float -> string
			cast = V_STRING;
			result = V_STRING;
			return true;
		}
		else if(left == V_FLOAT || right == V_FLOAT)
		{
			// int op float/float op int -> cast int to float
			cast = V_FLOAT;
			result = V_FLOAT;
			return true;
		}
		else
			return false;
	case S_SUB:
	case S_MUL:
	case S_DIV:
		if(left == V_STRING || right == V_STRING)
			return false; // can't do with string
		else if(left == right)
		{
			// no cast required (int/float)
			cast = V_VOID;
			result = left;
			return true;
		}
		else if(left == V_FLOAT || right == V_FLOAT)
		{
			// int op float/float op int -> cast int to float
			cast = V_FLOAT;
			result = V_FLOAT;
			return true;
		}
		else
			return false;
	case S_PLUS:
	case S_MINUS:
		// cast & result are unused
		if(left == V_INT || right == V_FLOAT)
			return true;
		else
			return false;
	default:
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

bool TryConstExpr(ParseNode* left, ParseNode* right, ParseNode* op, SYMBOL symbol)
{
	if(left->op == PUSH_INT && right->op == PUSH_INT)
	{
		// optimize const int expr
		int value;
		switch(symbol)
		{
		case S_ADD:
			value = left->value + right->value;
			break;
		case S_SUB:
			value = left->value - right->value;
			break;
		case S_MUL:
			value = left->value * right->value;
			break;
		case S_DIV:
			if(right->value == 0)
				value = 0;
			else
				value = left->value / right->value;
			break;
		default:
			assert(0);
			value = 0;
			break;
		}

		op->op = PUSH_INT;
		op->value = value;
	}
	else if(left->op == PUSH_FLOAT && right->op == PUSH_FLOAT)
	{
		// optimize const float expr
		float fvalue;
		switch(symbol)
		{
		case S_ADD:
			fvalue = left->fvalue + right->fvalue;
			break;
		case S_SUB:
			fvalue = left->fvalue - right->fvalue;
			break;
		case S_MUL:
			fvalue = left->fvalue * right->fvalue;
			break;
		case S_DIV:
			if(right->fvalue == 0.f)
				fvalue = 0.f;
			else
				fvalue = left->fvalue / right->fvalue;
			break;
		default:
			assert(0);
			fvalue = 0;
			break;
		}

		op->op = PUSH_FLOAT;
		op->fvalue = fvalue;
	}
	else
		return false;


	left->Free();
	right->Free();
	return true;
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
			c = strchr2(c, "+-*/()");
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
					{
						if(c == '+')
							symbol = S_PLUS;
						else if(c == '-')
							symbol = S_MINUS;
						else
							t.Unexpected();
						left = LEFT_UNARY;
					}
					break;
				case LEFT_UNARY:
					t.Unexpected();
				case LEFT_ITEM:
					symbol = CharToSymbol(c);
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
				VAR_TYPE unused;
				if(!CanOp(sn.symbol, node->type, V_VOID, unused, unused))
					t.Throw("Invalid type '%s' for operation '%s'.", var_name[node->type], si.name);
				if(sn.symbol == S_MINUS)
				{
					if(node->op == PUSH_INT || node->op == PUSH_FLOAT)
					{
						// modify const instead of adding op
						if(node->op == PUSH_INT)
							node->value = -node->value;
						else
							node->fvalue = -node->fvalue;
						stack2.push_back(node);
					}
					else
					{
						ParseNode* parent = ParseNode::Get();
						parent->op = NEG;
						parent->type = node->type;
						parent->push(node);
						stack2.push_back(parent);
					}
				}
				else
					stack2.push_back(node);
			}
			else if(si.args == 2)
			{
				ParseNode* right = stack2.back();
				stack2.pop_back();
				ParseNode* left = stack2.back();
				stack2.pop_back();

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
	ParseVar* old_var = FindVar(name);
	if(old_var)
		t.Throw("Variable already declared.");
	ParseVar* var = ParseVar::Get();
	var->name = name;
	var->type = type;
	var->index = vars.size();
	vars.push_back(var);
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
	node->op = POP_VAR;
	node->type = V_VOID;
	node->value = var->index;
	node->push(expr);
	return node;
}

ParseNode* ParseLine()
{
	if(t.IsKeywordGroup(G_VAR))
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
			node->op = GROUP;
			node->type = V_VOID;
			node->childs = nodes;
			return node;
		}
	}
	else if(t.IsItem())
	{
		const string& item = t.GetItem();
		ParseVar* var = FindVar(item);
		if(var)
		{
			// var assign
			t.Next();

			// op or =
			char c = t.MustGetSymbol("+-*/=");
			t.Next();
			if(c != '=')
			{
				t.AssertSymbol('=');
				t.Next();
			}

			// item
			ParseNode* child = ParseExpr(';');

			// ;
			t.AssertSymbol(';');
			t.Next();

			ParseNode* node = ParseNode::Get();
			node->op = POP_VAR;
			node->type = V_VOID;
			node->value = var->index;
			if(c == '=')
			{
				if(!TryCast(child, var->type))
					t.Throw("Can't assign '%s' to variable '%s %s'.", var_name[child->type], var_name[var->type], var->name.c_str());
				node->push(child);
			}
			else
			{
				SYMBOL symbol = CharToSymbol(c);
				SymbolInfo& si = symbols[symbol];
				VAR_TYPE cast, result;
				if(!CanOp(symbol, var->type, child->type, cast, result))
					t.Throw("Invalid types '%s' and '%s' for operation '%s'.", var_name[var->type], var_name[child->type], si.name);

				ParseNode* left = ParseNode::Get();
				left->op = PUSH_VAR;
				left->value = var->index;
				left->type = var->type;

				Cast(left, cast);
				Cast(child, cast);

				ParseNode* op = ParseNode::Get();
				op->op = (Op)si.op;
				op->type = result;
				op->push(left);
				op->push(child);

				Cast(op, var->type);
				node->push(op);
			}

			return node;
		}
	}

	ParseNode* node = ParseExpr(';');

	// ;
	t.AssertSymbol(';');
	t.Next();

	return node;
}

void ToCode(vector<int>& code, ParseNode* node)
{
	if(node->op == GROUP)
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
	case PUSH_VAR:
	case POP_VAR:
	case CALL:
	case CAST:
		code.push_back(node->op);
		code.push_back(node->value);
		break;
	case PUSH_FLOAT:
		code.push_back(node->op);
		code.push_back(*(int*)&node->fvalue);
		break;
	case NEG:
	case ADD:
	case SUB:
	case MUL:
	case DIV:
		code.push_back(node->op);
		break;
	default:
		assert(0);
		break;
	}
}

void ParseCode(vector<int>& code)
{
	ParseNode* node = ParseNode::Get();
	node->op = GROUP;
	node->type = V_VOID;

	t.Next();
	while(!t.IsEof())
	{
		ParseNode* child = ParseLine();
		if(child)
			node->push(child);
	}

	// codify
	ToCode(code, node);
	code.push_back(RET);
}

bool Parse(ParseContext& ctx)
{
	try
	{
		t.FromString(ctx.input);

		ParseCode(ctx.code);

		ctx.strs = strs;
		ctx.vars = vars.size();

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
}

void CleanupParser()
{
	ParseVar::Free(vars);
	pcode.clear();
	strs.clear();
}
