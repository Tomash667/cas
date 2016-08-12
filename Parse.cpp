#include "Pch.h"
#include "Base.h"
#include "Op.h"
#include "Function.h"
#include "Parse.h"
#include "Cas.h"


void ParseArgs(vector<ParseNode*>& nodes);
ParseNode* ParseBlock(ParseFunction* f = nullptr);
ParseNode* ParseExpr(char end, char end2 = 0, int* type = nullptr);
ParseNode* ParseLineOrBlock();


FOUND FindItem(const string& id, Found& found)
{
	if(current_type)
	{
		found.member = current_type->FindMember(id, found.member_index);
		if(found.member)
			return F_MEMBER;

		AnyFunction f = current_type->FindFunction(id);
		if(f)
		{
			if(f.is_parse)
			{
				found.func = f.f;
				return F_FUNC;
			}
			else
			{
				found.ufunc = f.pf;
				return F_USER_FUNC;
			}
		}
	}

	Function* func = Function::Find(id);
	if(func)
	{
		found.func = func;
		return F_FUNC;
	}

	for(ParseFunction* ufunc : ufuncs)
	{
		if(ufunc->name == id && ufunc->type == V_VOID)
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

void FindAllFunctionOverloads(const string& name, vector<AnyFunction>& items)
{
	for(Function* f : functions)
	{
		if(f->name == name && f->type == V_VOID)
			items.push_back(f);
	}

	for(ParseFunction* pf : ufuncs)
	{
		if(pf->name == name && pf->type == V_VOID)
			items.push_back(pf);
	}

	if(current_type)
	{
		for(ParseFunction* pf : current_type->ufuncs)
		{
			if(pf->name == name)
				items.push_back(pf);
		}
	}
}

void FindAllFunctionOverloads(Type* type, const string& name, vector<AnyFunction>& funcs)
{
	for(Function* f : type->funcs)
	{
		if(f->name == name)
			funcs.push_back(f);
	}

	for(ParseFunction* pf : type->ufuncs)
	{
		if(pf->name == name)
			funcs.push_back(pf);
	}
}

void FindAllCtors(Type* type, vector<AnyFunction>& funcs)
{
	for(Function* f : type->funcs)
	{
		if(f->special == SF_CTOR)
			funcs.push_back(f);
	}

	for(ParseFunction* pf : type->ufuncs)
	{
		if(pf->special == SF_CTOR)
			funcs.push_back(pf);
	}
}

AnyFunction FindEqualFunction(ParseFunction* pf)
{
	for(Function* f : functions)
	{
		if(f->name == pf->name && f->Equal(*pf))
			return f;
	}

	for(ParseFunction* f : ufuncs)
	{
		if(f->name == pf->name && f->Equal(*pf))
			return f;
	}

	return nullptr;
}

void CheckFindItem(const string& id, bool is_func)
{
	Found found;
	FOUND found_type = FindItem(id, found);
	if(found_type != F_NONE && !(is_func && (found_type == F_FUNC || found_type == F_USER_FUNC)))
		t.Throw("Name '%s' already used as %s.", id.c_str(), found.ToString(found_type));
}

bool TryConstCast(ParseNode* node, int type)
{
	// can cast only const literal
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

void Cast(ParseNode*& node, VarType type)
{
	bool need_cast = (node->type != type.core);
	int need_ref = 0; // -1 deref, 0-none, 1-take ref
	if(type.special == SV_NORMAL)
	{
		// require value type
		if(node->ref == REF_YES)
			need_ref = -1; // deref
	}
	else
	{
		// require reference to value type
		if(node->ref == REF_MAY)
			need_ref = 1; // take ref
	}

	// no cast required?
	if(!need_cast && need_ref == 0)
		return;

	// can const cast?
	if(TryConstCast(node, type.core))
		return;

	if(need_ref == -1)
	{
		// dereference
		ParseNode* deref = ParseNode::Get();
		deref->op = DEREF;
		deref->type = node->type;
		deref->ref = REF_NO;
		deref->push(node);
		node = deref;
	}
	else if(need_ref == 1)
	{
		// take address
		assert(node->op == PUSH_LOCAL || node->op == PUSH_GLOBAL || node->op == PUSH_ARG || node->op == PUSH_MEMBER || node->op == PUSH_THIS_MEMBER);
		node->op = Op(node->op + 1);
		node->ref = REF_YES;
	}
	
	// cast
	if(need_cast)
	{
		ParseNode* cast = ParseNode::Get();
		cast->op = CAST;
		cast->value = type.core;
		cast->type = type.core;
		cast->ref = REF_NO;
		cast->push(node);
		node = cast;
	}
}

// -1 - can't cast, 0 - no cast required, 1 - can cast
int MayCast(ParseNode* node, VarType type)
{
	// can't cast from void
	if(node->type == V_VOID)
		return -1;

	// no implicit cast from string to bool/int/float
	if(node->type == V_STRING && (type.core == V_BOOL || type.core == V_INT || type.core == V_FLOAT))
		return -1;

	bool cast = (node->type != type.core);
	// can't cast class
	if(cast && (node->type >= V_CLASS || type.core >= V_CLASS))
		return -1;

	if(type.special == SV_NORMAL)
	{
		// require value type
		if(node->ref == REF_YES)
			return 1; // dereference (and cast if required)
		else
			return cast ? 1 : 0; // cast if required
	}
	else
	{
		// require reference to value type
		if(node->ref == REF_YES)
			return (cast ? -1 : 0); // pass reference or can't cast to different type
		else if(node->ref == REF_MAY)
			return (cast ? -1 : 1); // take address or can't cast to different type
		else
			return -1; // can't take address
	}
}

// used in var assignment, passing argument to function
bool TryCast(ParseNode*& node, VarType type)
{
	int c = MayCast(node, type);
	if(c == 0)
		return true;
	else if(c == -1)
		return false;
	Cast(node, type);
	return true;
}

// 0 - don't match, 1 - require cast, 2 - match
int MatchFunctionCall(ParseNode* node, CommonFunction& f, bool is_parse)
{
	uint offset = 0;
	if((current_type && f.type == current_type->index) || (f.special == SF_CTOR && is_parse))
		++offset;

	if(node->childs.size() + offset > f.arg_infos.size() || node->childs.size() + offset < f.required_args)
		return 0;

	bool require_cast = false;
	for(uint i = 0; i < node->childs.size(); ++i)
	{
		int c = MayCast(node->childs[i], f.arg_infos[i + offset].type);
		if(c == -1)
			return 0;
		else if(c == 1)
			require_cast = true;
	}

	return (require_cast ? 1 : 2);
}

void ApplyFunctionCall(ParseNode* node, vector<AnyFunction>& funcs, Type* type, bool ctor)
{
	assert(!funcs.empty());

	int match_level = 0;
	vector<AnyFunction> match;

	for(AnyFunction& f : funcs)
	{
		int m = MatchFunctionCall(node, *f.f, f.is_parse);
		if(m == match_level)
			match.push_back(f);
		else if(m > match_level)
		{
			match.clear();
			match_level = m;
			match.push_back(f);
		}
	}
	assert(!match.empty());

	if(match.size() >= 2u || match_level == 0)
	{
		LocalString s;
		if(match.size() >= 2u)
			s = "Ambiguous call to overloaded ";
		else
			s = "No matching call to ";
		if(type)
			s += Format("method '%s.", type->name.c_str());
		else
			s += "function '";
		s += match.front().f->name;
		s += '\'';
		uint var_offset = ((type && !ctor) ? 1 : 0);
		if(node->childs.size() > var_offset)
		{
			s += " with arguments (";
			bool first = true;
			for(uint i = var_offset; i < node->childs.size(); ++i)
			{
				if(!first)
					s += ',';
				first = false;
				s += types[node->childs[i]->type]->name;
			}
			s += ')';
		}
		s += ", could be";
		if(match.size() >= 2u)
		{
			s += ':';
			for(AnyFunction& f : match)
			{
				s += "\n\t";
				s += f.f->GetName();
			}
		}
		else
			s += Format(" '%s'.", match.front().f->GetName());
		t.Throw(s->c_str());
	}
	else
	{
		AnyFunction f = match.front();
		CommonFunction& cf = *f.f;
		bool callu_ctor = false;

		if(current_type && cf.type == current_type->index)
		{
			// push this
			ParseNode* thi = ParseNode::Get();
			thi->op = PUSH_ARG;
			thi->type = cf.type;
			thi->value = 0;
			thi->ref = REF_NO;
			node->childs.insert(node->childs.begin(), thi);
		}
		else if(cf.special == SF_CTOR && f.is_parse)
		{
			// user constructor call
			callu_ctor = true;
		}

		// cast params
		if(match_level == 1)
		{
			for(uint i = 0; i < node->childs.size(); ++i)
				Cast(node->childs[i], cf.arg_infos[i].type);
		}

		// fill default params
		for(uint i = node->childs.size() + (callu_ctor ? 1 : 0); i < cf.arg_infos.size(); ++i)
		{
			ArgInfo& arg = cf.arg_infos[i];
			ParseNode* n = ParseNode::Get();
			n->type = arg.type.core;
			assert(arg.type.special == SV_NORMAL);
			switch(arg.type.core)
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

		// apply type
		node->op = (f.is_parse ? (callu_ctor ? CALLU_CTOR : CALLU) : CALL);
		node->type = cf.result.core;
		node->ref = (cf.result.special == SV_NORMAL ? REF_NO : REF_YES);
		node->value = cf.index;
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
		node->ref = REF_NO;
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
		node->ref = REF_NO;
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
		node->ref = REF_NO;
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
		node->ref = REF_NO;
		return node;
	}
	else
		t.Unexpected();
}

ParseNode* ParseItem(int* type = nullptr)
{
	if(t.IsKeywordGroup(G_VAR) || type)
	{
		VarType var_type(V_VOID);
		if(type)
			var_type.core = *type;
		else
		{
			var_type.core = t.GetKeywordId(G_VAR);
			t.Next();
		}

		t.AssertSymbol('(');
		Type* rtype = types[var_type.core];
		if(!rtype->have_ctor)
			t.Throw("Type '%s' don't have constructor.", rtype->name.c_str());
		ParseNode* node = ParseNode::Get();
		node->ref = REF_NO;
		ParseArgs(node->childs);
		vector<AnyFunction> funcs;
		FindAllCtors(rtype, funcs);
		ApplyFunctionCall(node, funcs, rtype, true);
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
				node->type = var->type.core;
				node->value = var->index;
				node->ref = (var->type.special == SV_NORMAL ? REF_MAY : REF_YES);
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
					if(current_type)
						node->value++;
					break;
				}
				t.Next();
				return node;
			}
		case F_FUNC:
		case F_USER_FUNC:
			{
				vector<AnyFunction> funcs;
				FindAllFunctionOverloads(id, funcs);
				t.Next();

				ParseNode* node = ParseNode::Get();
				node->ref = REF_NO;

				ParseArgs(node->childs);
				ApplyFunctionCall(node, funcs, nullptr, false);

				return node;
			}
		case F_MEMBER:
			{
				ParseNode* node = ParseNode::Get();
				node->op = PUSH_THIS_MEMBER;
				node->type = found.member->type;
				node->value = found.member_index;
				node->ref = REF_MAY;
				t.Next();
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
	S_BIT_AND, "bit and", 10, true, 2, BIT_AND, ST_NONE,
	S_BIT_OR, "bit or", 12, true, 2, BIT_OR, ST_NONE,
	S_BIT_XOR, "bit xor", 11, true, 2, BIT_XOR, ST_NONE,
	S_BIT_LSHIFT, "bit left shift", 7, true, 2, BIT_LSHIFT, ST_NONE,
	S_BIT_RSHIFT, "bit right shift", 7, true, 2, BIT_RSHIFT, ST_NONE,
	S_PLUS, "unary plus", 3, false, 1, NOP, ST_NONE,
	S_MINUS, "unary minus", 3, false, 1, NEG, ST_NONE,
	S_EQUAL, "equal", 9, true, 2, EQ, ST_NONE,
	S_NOT_EQUAL, "not equal", 9, true, 2, NOT_EQ, ST_NONE,
	S_GREATER, "greater", 8, true, 2, GR, ST_NONE,
	S_GREATER_EQUAL, "greater equal", 8, true, 2, GR_EQ, ST_NONE,
	S_LESS, "less", 8, true, 2, LE, ST_NONE,
	S_LESS_EQUAL, "less equal", 8, true, 2, LE_EQ, ST_NONE,
	S_AND, "and", 13, true, 2, AND, ST_NONE,
	S_OR, "or", 14, true, 2, OR, ST_NONE,
	S_NOT, "not", 3, false, 1, NOT, ST_NONE,
	S_BIT_NOT, "bit not", 3, false, 1, BIT_NOT, ST_NONE,
	S_MEMBER_ACCESS, "member access", 2, true, 2, NOP, ST_NONE,
	S_ASSIGN, "assign", 15, false, 2, NOP, ST_ASSIGN,
	S_ASSIGN_ADD, "assign add", 15, false, 2, S_ADD, ST_ASSIGN,
	S_ASSIGN_SUB, "assign subtract", 15, false, 2, S_SUB, ST_ASSIGN,
	S_ASSIGN_MUL, "assign multiply", 15, false, 2, S_MUL, ST_ASSIGN,
	S_ASSIGN_DIV, "assign divide", 15, false, 2, S_DIV, ST_ASSIGN,
	S_ASSIGN_MOD, "assign modulo", 15, false, 2, S_MOD, ST_ASSIGN,
	S_ASSIGN_BIT_AND, "assign bit and", 15, false, 2, S_BIT_AND, ST_ASSIGN,
	S_ASSIGN_BIT_OR, "assign bit or", 15, false, 2, S_BIT_OR, ST_ASSIGN,
	S_ASSIGN_BIT_XOR, "assign bit xor", 15, false, 2, S_BIT_XOR, ST_ASSIGN,
	S_ASSIGN_BIT_LSHIFT, "assign bit left shift", 15, false, 2, S_BIT_LSHIFT, ST_ASSIGN,
	S_ASSIGN_BIT_RSHIFT, "assign bit right shift", 15, false, 2, S_BIT_RSHIFT, ST_ASSIGN,
	S_PRE_INC, "pre increment", 3, false, 1, INC, ST_INC_DEC,
	S_PRE_DEC, "pre decrement", 3, false, 1, DEC, ST_INC_DEC,
	S_POST_INC, "post increment", 2, true, 1, INC, ST_INC_DEC,
	S_POST_DEC, "post decrement", 2, true, 1, DEC, ST_INC_DEC,
	S_IS, "reference equal", 9, true, 2, IS, ST_NONE,
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

bool CanOp(SYMBOL symbol, int left, int right, int& cast, int& result)
{
	if(left == V_VOID)
		return false;
	if(right == V_VOID && symbols[symbol].args != 1)
		return false;
	if((left >= V_CLASS || right >= V_CLASS) && symbol != S_IS)
		return false;

	int type;
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
		if(left == V_INT || left == V_FLOAT)
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
	case S_BIT_AND:
	case S_BIT_OR:
	case S_BIT_XOR:
	case S_BIT_LSHIFT:
	case S_BIT_RSHIFT:
		// not allowed for string, cast other to int
		if(left == V_STRING || right == V_STRING)
			return false;
		cast = V_INT;
		result = V_INT;
		return true;
	case S_BIT_NOT:
		// not allowed for string, cast other to int
		if(left == V_STRING)
			return false;
		cast = V_INT;
		result = V_INT;
		return true;
	case S_IS:
		if(left == V_STRING && right == V_STRING)
		{
			cast = V_STRING;
			result = V_BOOL;
			return true;
		}
		else if(left >= V_CLASS && left == right)
		{
			cast = left;
			result = V_BOOL;
			return true;
		}
		else
			return false;
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
	else if(symbol == S_BIT_NOT)
	{
		if(node->op == PUSH_INT)
		{
			node->value = ~node->value;
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
		case S_BIT_AND:
			op->value = (left->value & right->value);
			op->op = PUSH_INT;
			break;
		case S_BIT_OR:
			op->value = (left->value | right->value);
			op->op = PUSH_INT;
			break;
		case S_BIT_XOR:
			op->value = (left->value ^ right->value);
			op->op = PUSH_INT;
			break;
		case S_BIT_LSHIFT:
			op->value = (left->value << right->value);
			op->op = PUSH_INT;
			break;
		case S_BIT_RSHIFT:
			op->value = (left->value >> right->value);
			op->op = PUSH_INT;
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
	BS_BIT_AND, "&", S_INVALID, S_INVALID, S_INVALID, S_BIT_AND,
	BS_BIT_OR, "|", S_INVALID, S_INVALID, S_INVALID, S_BIT_OR,
	BS_BIT_XOR, "^", S_INVALID, S_INVALID, S_INVALID, S_BIT_XOR,
	BS_BIT_LSHIFT, "<<", S_INVALID, S_INVALID, S_INVALID, S_BIT_LSHIFT,
	BS_BIT_RSHIFT, ">>", S_INVALID, S_INVALID, S_INVALID, S_BIT_RSHIFT,
	BS_EQUAL, "==", S_INVALID, S_INVALID, S_INVALID, S_EQUAL,
	BS_NOT_EQUAL, "!=", S_INVALID, S_INVALID, S_INVALID, S_NOT_EQUAL,
	BS_GREATER, ">", S_INVALID, S_INVALID, S_INVALID, S_GREATER,
	BS_GREATER_EQUAL, ">=", S_INVALID, S_INVALID, S_INVALID, S_GREATER_EQUAL,
	BS_LESS, "<", S_INVALID, S_INVALID, S_INVALID, S_LESS,
	BS_LESS_EQUAL, "<=", S_INVALID, S_INVALID, S_INVALID, S_LESS_EQUAL,
	BS_AND, "&&", S_INVALID, S_INVALID, S_INVALID, S_AND,
	BS_OR, "||", S_INVALID, S_INVALID, S_INVALID, S_OR,
	BS_NOT, "!", S_NOT, S_INVALID, S_INVALID, S_INVALID,
	BS_BIT_NOT, "~", S_INVALID, S_INVALID, S_INVALID, S_BIT_NOT,
	BS_ASSIGN, "=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN,
	BS_ASSIGN_ADD, "+=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_ADD,
	BS_ASSIGN_SUB, "-=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_SUB,
	BS_ASSIGN_MUL, "*=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_MUL,
	BS_ASSIGN_DIV, "/=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_DIV,
	BS_ASSIGN_MOD, "%=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_MOD,
	BS_ASSIGN_BIT_AND, "&=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_BIT_AND,
	BS_ASSIGN_BIT_OR, "|=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_BIT_OR,
	BS_ASSIGN_BIT_XOR, "^=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_BIT_XOR,
	BS_ASSIGN_BIT_LSHIFT, "<<=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_BIT_LSHIFT,
	BS_ASSIGN_BIT_RSHIFT, ">>=", S_INVALID, S_INVALID, S_INVALID, S_ASSIGN_BIT_RSHIFT,
	BS_DOT, ".", S_INVALID, S_INVALID, S_INVALID, S_MEMBER_ACCESS,
	BS_INC, "++", S_INVALID, S_PRE_INC, S_POST_INC, S_INVALID,
	BS_DEC, "--", S_INVALID, S_PRE_DEC, S_POST_DEC, S_INVALID,
	BS_IS, "is", S_INVALID, S_INVALID, S_INVALID, S_IS
};

BASIC_SYMBOL GetSymbol()
{
	if(t.IsKeyword(K_IS, G_KEYWORD))
		return BS_IS;
	if(!t.IsSymbol())
		return BS_MAX;
	char c = t.GetSymbol();
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
		else if(t.PeekSymbol('>'))
		{
			if(t.PeekSymbol('='))
				return BS_ASSIGN_BIT_RSHIFT;
			else
				return BS_BIT_RSHIFT;
		}
		else
			return BS_GREATER;
	case '<':
		if(t.PeekSymbol('='))
			return BS_LESS_EQUAL;
		else if(t.PeekSymbol('<'))
		{
			if(t.PeekSymbol('='))
				return BS_ASSIGN_BIT_LSHIFT;
			else
				return BS_BIT_LSHIFT;
		}
		else
			return BS_LESS;
	case '&':
		if(t.PeekSymbol('&'))
			return BS_AND;
		else if(t.PeekSymbol('='))
			return BS_ASSIGN_BIT_AND;
		else
			return BS_BIT_AND;
	case '|':
		if(t.PeekSymbol('|'))
			return BS_OR;
		else if(t.PeekSymbol('='))
			return BS_ASSIGN_BIT_OR;
		else
			return BS_BIT_OR;
	case '^':
		if(t.PeekSymbol('='))
			return BS_ASSIGN_BIT_XOR;
		else
			return BS_BIT_XOR;
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

BASIC_SYMBOL ParseExprPart(vector<SymbolOrNode>& exit, vector<SYMBOL>& stack, int* type)
{
	BASIC_SYMBOL symbol = BS_MAX;

	if(!type)
	{
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
	}
	else
		exit.push_back(ParseItem(type));

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

ParseNode* ParseExpr(char end, char end2, int* type)
{
	vector<SymbolOrNode> exit;
	vector<SYMBOL> stack;

	while(true)
	{
		BASIC_SYMBOL left = ParseExprPart(exit, stack, type);
		type = nullptr;
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
				node->type = V_VOID;
				node->str = str;
				node->ref = REF_NO;
				if(t.IsSymbol('('))
				{
					ParseArgs(node->childs);
					node->pseudo_op = OBJ_FUNC;
				}
				else
					node->pseudo_op = OBJ_MEMBER;
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
					// unrary operator
					int cast, result;
					if(!CanOp(sn.symbol, node->type, V_VOID, cast, result))
						t.Throw("Invalid type '%s' for operation '%s'.", types[node->type]->name.c_str(), si.name);
					Cast(node, VarType(cast));
					if(!TryConstExpr1(node, si.symbol) && si.op != NOP)
					{
						ParseNode* op = ParseNode::Get();
						op->op = (Op)si.op;
						op->type = result;
						op->push(node);
						op->ref = REF_NO;
						node = op;
					}
					stack2.push_back(node);
				}
				else
				{
					// inc dec
					assert(si.type == ST_INC_DEC);
					if(node->type != V_INT && node->type != V_FLOAT)
						t.Throw("Invalid type '%s' for operation '%s'.", types[node->type]->name.c_str(), si.name);

					bool pre = (si.symbol == S_PRE_INC || si.symbol == S_PRE_DEC);
					bool inc = (si.symbol == S_PRE_INC || si.symbol == S_POST_INC);
					Op oper = (inc ? INC : DEC);

					ParseNode* op = ParseNode::Get();
					op->pseudo_op = INTERNAL_GROUP;
					op->type = node->type;

					if(node->ref != REF_YES)
					{
						Op set_op;
						switch(node->op)
						{
						case PUSH_LOCAL:
							set_op = SET_LOCAL;
							break;
						case PUSH_GLOBAL:
							set_op = SET_GLOBAL;
							break;
						case PUSH_ARG:
							set_op = SET_ARG;
							break;
						case PUSH_MEMBER:
							set_op = SET_MEMBER;
							break;
						case PUSH_THIS_MEMBER:
							set_op = SET_THIS_MEMBER;
							break;
						default:
							t.Throw("Operation '%s' require variable.", si.name);
						}

						if(pre)
						{
							/* ++a
							push a; a
							inc; a+1
							set; a+1  a->a+1
							*/
							op->push(node);
							op->push(oper);
							op->push(set_op, node->value);
						}
						else
						{
							/* a++
							push a; a
							push; a,a
							inc; a,a+1
							set; a,a+1  a->a+1
							pop; a
							*/
							op->push(node);
							op->push(PUSH);
							op->push(oper);
							op->push(set_op, node->value);
							op->push(POP);
						}
					}
					else
					{
						if(pre)
						{
							/* ++a (a is reference)
							push a; a
							push; a,a
							deref; a,[a]
							inc; a,[a]+1
							set_arg; [a]+1  a->a+1
							*/
							op->push(node);
							op->push(PUSH);
							op->push(DEREF);
							op->push(INC);
							op->push(SET_ADR);
						}
						else
						{
							/* a++ (a is reference)
							push a; a
							deref; [a]
							push a; [a],a
							push; [a],a,a
							deref; [a],a,[a]
							inc; [a],a,[a]+1
							set_adr; [a],[a]+1  a->a+1
							pop; [a]
							*/
							op->push(node);
							op->push(DEREF);
							op->push(node);
							op->push(PUSH);
							op->push(DEREF);
							op->push(INC);
							op->push(SET_ADR);
							op->push(POP);
						}
					}

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
					if(left->type == V_VOID)
						t.Throw("Invalid member access for type 'void'.");
					Type* type = types[left->type];
					if(right->pseudo_op == OBJ_FUNC)
					{
						vector<AnyFunction> funcs;
						FindAllFunctionOverloads(type, *right->str, funcs);
						if(funcs.empty())
							t.Throw("Missing method '%s' for type '%s'.", right->str->c_str(), type->name.c_str());
						StringPool.Free(right->str);

						ParseNode* node = ParseNode::Get();
						node->ref = REF_NO;
						node->push(left);
						for(ParseNode* n : right->childs)
							node->push(n);

						ApplyFunctionCall(node, funcs, type, false);

						right->childs.clear();
						right->Free();
						stack2.push_back(node);
					}
					else
					{
						assert(right->pseudo_op == OBJ_MEMBER);
						int m_index;
						Member* m = type->FindMember(*right->str, m_index);
						if(!m)
							t.Throw("Missing member '%s' for type '%s'.", right->str->c_str(), type->name.c_str());
						StringPool.Free(right->str);
						ParseNode* node = ParseNode::Get();
						node->op = PUSH_MEMBER;
						node->type = m->type;
						node->value = m_index;
						node->ref = REF_MAY;
						node->push(left);
						right->Free();
						stack2.push_back(node);
					}
				}
				else if(si.type == ST_ASSIGN)
				{
					if(left->op != PUSH_LOCAL && left->op != PUSH_GLOBAL && left->op != PUSH_ARG && left->op != PUSH_MEMBER && left->op != PUSH_THIS_MEMBER
						&& left->ref != REF_YES)
						t.Throw("Can't assign, left value must be variable.");

					ParseNode* set = ParseNode::Get();
					if(left->ref != REF_YES)
					{
						switch(left->op)
						{
						case PUSH_LOCAL:
							set->op = SET_LOCAL;
							break;
						case PUSH_GLOBAL:
							set->op = SET_GLOBAL;
							break;
						case PUSH_ARG:
							set->op = SET_ARG;
							break;
						case PUSH_MEMBER:
							set->op = SET_MEMBER;
							break;
						case PUSH_THIS_MEMBER:
							set->op = SET_THIS_MEMBER;
							break;
						default:
							assert(0);
							break;
						}
						set->value = left->value;
						set->type = left->type;
						set->ref = REF_NO;

						if(si.op == NOP)
						{
							// assign
							if(!TryCast(right, VarType(left->type)))
								t.Throw("Can't assign '%s' to type '%s'.", types[right->type]->name.c_str(), types[set->type]->name.c_str());
							set->push(right);
							if(left->op == PUSH_MEMBER)
								set->push(left->childs);
						}
						else
						{
							// compound assign
							int cast, result;
							if(!CanOp((SYMBOL)si.op, left->type, right->type, cast, result))
								t.Throw("Invalid types '%s' and '%s' for operation '%s'.", types[left->type]->name.c_str(), types[right->type]->name.c_str(),
									si.name);

							Cast(left, VarType(cast));
							Cast(right, VarType(cast));

							ParseNode* op = ParseNode::Get();
							op->op = (Op)symbols[si.op].op;
							op->type = result;
							op->ref = REF_NO;
							op->push(left);
							op->push(right);

							if(!TryCast(op, VarType(set->type)))
								t.Throw("Can't cast return value from '%s' to '%s' for operation '%s'.", types[op->type]->name.c_str(),
									types[set->type]->name.c_str(), si.name);
							set->push(op);
							if(left->op == PUSH_MEMBER)
								set->push(left->childs);
						}
					}
					else
					{
						set->type = left->type;
						set->ref = REF_NO;

						if(si.op == NOP)
						{
							// assign
							if(!TryCast(right, VarType(left->type)))
								t.Throw("Can't assign '%s' to type '%s'.", types[right->type]->name.c_str(), types[left->type]->name.c_str());
							set->op = SET_ADR;
							set->push(left);
							set->push(right);
						}
						else
						{
							// compound assign
							int cast, result;
							if(!CanOp((SYMBOL)si.op, left->type, right->type, cast, result))
								t.Throw("Invalid types '%s' and '%s' for operation '%s'.", types[left->type]->name.c_str(), types[right->type]->name.c_str(),
									si.name);

							ParseNode* real_left = left;
							Cast(left, VarType(cast));
							Cast(right, VarType(cast));

							ParseNode* op = ParseNode::Get();
							op->op = (Op)symbols[si.op].op;
							op->type = result;
							op->ref = REF_NO;
							op->push(left);
							op->push(right);

							if(!TryCast(op, VarType(set->type)))
								t.Throw("Can't cast return value from '%s' to '%s' for operation '%s'.", types[op->type]->name.c_str(),
									types[set->type]->name.c_str(), si.name);
							set->push(real_left);
							set->push(op);
							set->op = SET_ADR;
						}
					}

					stack2.push_back(set);
				}
				else
				{
					int cast, result;
					if(!CanOp(si.symbol, left->type, right->type, cast, result))
						t.Throw("Invalid types '%s' and '%s' for operation '%s'.", types[left->type]->name.c_str(), types[right->type]->name.c_str(), si.name);

					Cast(left, VarType(cast));
					Cast(right, VarType(cast));

					ParseNode* op = ParseNode::Get();
					op->type = result;
					op->ref = REF_NO;

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

VarType GetVarType()
{
	if(!t.IsKeywordGroup(G_VAR))
		t.Unexpected("Expecting var type.");
	int type = t.GetKeywordId(G_VAR);
	t.Next();
	if(t.IsSymbol('&'))
	{
		Type* ty = types[type];
		if(ty->is_ref)
			t.Throw("Can't create reference to reference type '%s'.", ty->name.c_str());
		t.Next();
		return VarType(type, SV_REF);
	}
	else
		return VarType(type);
}

int GetVarTypeForMember()
{
	int type = t.MustGetKeywordId(G_VAR);
	if(type == V_VOID)
		t.Throw("Class member can't be void type.");
	else if(type == V_STRING || type >= V_CLASS)
		t.Throw("Class '%s' member not supported yet.", types[type]->name.c_str());
	t.Next();
	return type;
}

ParseVar* GetVar(ParseNode* node)
{
	switch(node->op)
	{
	case PUSH_GLOBAL:
	case PUSH_GLOBAL_REF:
		return main_block->vars[node->value];
	case PUSH_LOCAL:
	case PUSH_LOCAL_REF:
		assert(current_block);
		return current_block->GetVar(node->value);
	case PUSH_ARG:
	case PUSH_ARG_REF:
		assert(current_function);
		return current_function->args[node->value];
	default:
		assert(0);
		return nullptr;
	}
}

void ToCode(vector<int>& code, ParseNode* node, vector<uint>* break_pos)
{
	if(node->pseudo_op >= SPECIAL_OP)
	{
		switch(node->pseudo_op)
		{
		case IF:
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
			}
			break;
		case DO_WHILE:
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
			}
			break;
		case WHILE:
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
			}
			break;
		case FOR:
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
			}
			break;
		case GROUP:
			{
				for(ParseNode* n : node->childs)
				{
					ToCode(code, n, break_pos);
					if(n->type != V_VOID)
						code.push_back(POP);
				}
			}
			break;
		default:
			assert(0);
			break;
		}
		return;
	}
	
	for(ParseNode* n : node->childs)
		ToCode(code, n, break_pos);
	
	switch(node->op)
	{
	case INTERNAL_GROUP:
		break;
	case PUSH_INT:
	case PUSH_STRING:
	case CALL:
	case CALLU:
	case CALLU_CTOR:
	case CAST:
	case PUSH_LOCAL:
	case PUSH_LOCAL_REF:
	case PUSH_GLOBAL:
	case PUSH_GLOBAL_REF:
	case PUSH_ARG:
	case PUSH_ARG_REF:
	case PUSH_MEMBER:
	case PUSH_MEMBER_REF:
	case PUSH_THIS_MEMBER:
	case PUSH_THIS_MEMBER_REF:
	case SET_LOCAL:
	case SET_GLOBAL:
	case SET_ARG:
	case SET_MEMBER:
	case SET_THIS_MEMBER:
	case CTOR:
		code.push_back(node->op);
		code.push_back(node->value);
		break;
	case PUSH_FLOAT:
		code.push_back(node->op);
		code.push_back(*(int*)&node->fvalue);
		break;
	case PUSH:
	case POP:
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
	case INC:
	case DEC:
	case DEREF:
	case SET_ADR:
	case BIT_AND:
	case BIT_OR:
	case BIT_XOR:
	case BIT_LSHIFT:
	case BIT_RSHIFT:
	case BIT_NOT:
	case IS:
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

int GetReturnType(ParseNode* node)
{
	if(node->childs.empty())
		return V_VOID;
	else
		return node->childs.front()->type;
}

int CommonType(int a, int b)
{
	if(a == b)
		return a;
	if(a == V_VOID || b == V_VOID)
		return -1;
	if(a == V_FLOAT || b == V_FLOAT)
		return V_FLOAT;
	else if(a == V_INT || b == V_INT)
		return V_INT;
	else
	{
		assert(0);
		return -1;
	}
}



void CleanupParser()
{
	// cleanup old types
	for(uint i = builtin_types, count = types.size(); i<count; ++i)
	{
		Type* type = types[i];
		t.RemoveKeyword(type->name.c_str(), i, G_VAR);
		delete type;
	}
	types.resize(builtin_types);

	Str::Free(strs);
	DeleteElements(ufuncs);
}

enum EXT_VAR_TYPE
{
	V_FUNCTION = V_CLASS + 1,
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
	int cf = -2;

	cout << "DECOMPILE:\n";
	while(c != end)
	{
		if(cf == -2)
		{
			if(ctx.ufuncs.empty())
			{
				cf = -1;
				cout << "Main:\n";
			}
			else
			{
				cf = 0;
				cout << "Function " << ufuncs[0]->GetName(false) << ":\n";
			}
		}
		else if(cf != -1)
		{
			uint offset = c - ctx.code.data();
			if(offset >= ctx.entry_point)
			{
				cf = -1;
				cout << "Main:\n";
			}
			else if(cf + 1 < (int)ctx.ufuncs.size())
			{
				if(offset >= ctx.ufuncs[cf + 1].pos)
				{
					++cf;
					cout << "Function " << ufuncs[cf]->GetName(false) << ":\n";
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
					cout << Format("\t[%d %d] %s \"%s\"\n", (int)op, str_idx, opi.name, ctx.strs[str_idx]->s.c_str());
				}
				break;
			case V_FUNCTION:
				{
					int f_idx = *c++;
					cout << Format("\t[%d %d] %s %s\n", (int)op, f_idx, opi.name, functions[f_idx]->GetName(false));
				}
				break;
			case V_USER_FUNCTION:
				{
					int f_idx = *c++;
					cout << Format("\t[%d %d] %s %s\n", (int)op, f_idx, opi.name, ufuncs[f_idx]->GetName(false));
				}
				break;
			case V_TYPE:
				{
					int type = *c++;
					cout << Format("\t[%d %d] %s %s\n", (int)op, type, opi.name, types[type]->name.c_str());
				}
				break;
			}
		}
	}
	cout << "\n";
}

// func_decl
Function* ParseFuncDecl(cstring decl, Type* type)
{
	assert(decl);

	Function* f = new Function;

	try
	{
		t.FromString(decl);
		t.Next();

		f->result = GetVarType();

		if(type && type->index == f->result.core && t.IsSymbol('('))
		{
			// ctor
			f->special = SF_CTOR;
			f->name = type->name;
		}
		else
		{
			f->special = SF_NO;
			f->name = t.MustGetItem();
			t.Next();
		}

		t.AssertSymbol('(');
		t.Next();

		ParseFunctionArgs(f, false);

		t.AssertEof();
	}
	catch(Tokenizer::Exception& e)
	{
		handler(cas::Error, e.ToString());
		delete f;
		f = nullptr;
	}

	return f;
}

Type::~Type()
{
	DeleteElements(members);
}

AnyFunction Type::FindFunction(const string& name)
{
	for(Function* f : funcs)
	{
		if(f->name == name)
			return f;
	}

	for(ParseFunction* pf : ufuncs)
	{
		if(pf->name == name)
			return pf;
	}

	return nullptr;
}

AnyFunction Type::FindEqualFunction(Function& fc)
{
	for(Function* f : funcs)
	{
		if(f->name == fc.name && f->Equal(fc))
			return f;
	}

	for(ParseFunction* pf : ufuncs)
	{
		if(pf->name == name && pf->Equal(fc))
			return pf;
	}

	return nullptr;
}
