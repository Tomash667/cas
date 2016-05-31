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
	int value;
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

bool TryCast(ParseNode*& node, VAR_TYPE exp_type)
{
	VAR_TYPE type = node->type;
	if(type == exp_type)
		return true;
	if(exp_type == V_STRING && type == V_INT)
	{
		// cast int -> string
		ParseNode* cast = ParseNode::Get();
		cast->op = CAST;
		cast->value = exp_type;
		cast->type = exp_type;
		cast->push(node);
		node = cast;
		return true;
	}
	return false;
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

ParseNode* ParseExpr(char end, char end2)
{
	vector<SymbolOrNode> exit;
	vector<SYMBOL> stack;
	LEFT left = LEFT_NONE;

	while(true)
	{
		if(t.IsSymbol())
		{
			char c = t.GetSymbol();
			if(c == end || c == end2)
				break;
			c = strchr2(c, "+-*/");
			if(c == 0)
				t.Unexpected();
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
				if(node->type != V_INT)
					t.Throw("Invalid type '%s' for operation '%s'.", var_name[node->type], si.name);
				if(sn.symbol == S_MINUS)
				{
					if(node->op == PUSH_INT)
					{
						node->value = -node->value;
						stack2.push_back(node);
					}
					else
					{
						ParseNode* parent = ParseNode::Get();
						parent->op = NEG;
						parent->type = V_INT;
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

				if(left->type != V_INT || right->type != V_INT)
					t.Throw("Invalid types '%s' and '%s' for operation '%s'.", var_name[left->type], var_name[right->type], si.name);

				int value;
				switch(si.symbol)
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

				ParseNode* op = ParseNode::Get();
				op->type = V_INT;

				if(left->op == PUSH_INT && right->op == PUSH_INT)
				{
					// optimialization of const expression
					op->op = PUSH_INT;
					op->value = value;
					left->Free();
					right->Free();
				}
				else
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

ParseNode* ParseLine()
{
	ParseVar* var;
	if(t.IsKeywordGroup(G_VAR))
	{
		// var_type
		VAR_TYPE type = (VAR_TYPE)t.GetKeywordId();
		if(type == V_VOID)
			t.Throw("Can't declare void variable.");
		t.Next();

		// var_name
		const string& name = t.MustGetItem();
		ParseVar* old_var = FindVar(name);
		if(old_var)
			t.Throw("Variable already declared.");
		var = ParseVar::Get();
		var->name = name;
		var->type = type;
		var->index = vars.size();
		vars.push_back(var);
		t.Next();

		// ;
		t.AssertSymbol(';');
		t.Next();

		return nullptr;
	}
	else if(t.IsItem())
	{
		const string& item = t.GetItem();
		ParseVar* var = FindVar(item);
		if(var)
		{
			// var assign
			t.Next();

			// =
			t.AssertSymbol('=');
			t.Next();

			// item
			ParseNode* child = ParseExpr(';');

			// ;
			t.AssertSymbol(';');
			t.Next();

			ParseNode* node = ParseNode::Get();
			node->op = POP_VAR;
			node->type = V_VOID;
			node->value = var->index;
			node->push(child);

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
	vector<ParseNode*> nodes;
	t.Next();
	while(!t.IsEof())
	{
		ParseNode* node = ParseLine();
		if(node)
			nodes.push_back(node);
	}

	// codify
	for(ParseNode* n : nodes)
	{
		ToCode(code, n);
		if(n->type != V_VOID)
			code.push_back(POP);
	}

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
