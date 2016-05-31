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
	// [+-]
	bool neg = false;
	char c;
	if(t.IsSymbol("-+", &c))
	{
		t.Next();
		neg = (c == '-');
	}

	if(t.IsInt())
	{
		// int
		int val = t.GetInt();
		if(neg)
			val = -val;
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
			// var
			if(var->type != V_INT && neg)
				t.Unexpected();

			ParseNode* node = ParseNode::Get();
			node->op = PUSH_VAR;
			node->type = var->type;
			node->value = var->index;
			t.Next();
			if(!neg)
				return node;
			else
			{
				ParseNode* node2 = ParseNode::Get();
				node2->op = NEG;
				node2->type = V_INT;
				node2->push(node);
				return node2;
			}
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
					node->push(ParseItem());
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

			if(neg)
			{
				if(node->type != V_INT)
					t.Throw("Can't negate.");
				ParseNode* node2 = ParseNode::Get();
				node2->op = NEG;
				node2->type = V_INT;
				node2->push(node);
				return node2;
			}
			else
				return node;
		}
		else
			t.Unexpected();
	}
	else if(t.IsString())
	{
		// string
		if(neg)
			t.Unexpected();
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
			ParseNode* child = ParseItem();

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

	ParseNode* node = ParseItem();

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

void RegisterVarTypes()
{
	for(int i = 0; i < V_MAX; ++i)
		t.AddKeyword(var_name[i], i, G_VAR);
}

bool Parse(ParseContext& ctx)
{
	RegisterVarTypes();

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
