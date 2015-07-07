#include "Tokenizer.h"
#include <conio.h>
#include <iostream>
#include <ctime>

using std::vector;

enum Op
{
	add,
	sub,
	mul,
	o_div,
	mod,	
	neg,
	inc_pre,
	inc_post,
	dec_pre,
	dec_post,

	equal,
	not_equal,
	greater,
	greater_equal,
	less,
	less_equal,
	and,
	or,

	cast,

	locals,
	set_local,
	set_local_index,
	set_local_indexvar,

	push_local,
	push_local_ref,
	push_local_index,
	push_local_indexvar,
	push_local_index_ref,
	push_local_indexvar_ref,
	push_cstr,
	push_int,
	push_float,
	pop,

	call,
	jmp,
	jmp_if,

	ret
};

enum VarType
{
	VOID,
	INT,
	STR,
	BOOL,
	FLOAT,
	REF,
	ARRAY
};

inline cstring VarTypeToString(VarType type)
{
	switch(type)
	{
	case VOID:
		return "void";
	case INT:
		return "int";
	case STR:
		return "string";
	case BOOL:
		return "bool";
	case FLOAT:
		return "float";
	case REF:
		return "ref";
	case ARRAY:
		return "array";
	default:
		assert(0);
		return "???";
	}
}

struct Str
{
	string s;
	int refs;
};

struct _StrPool
{
	vector<Str*> v;

	inline Str* Get()
	{
		Str* s;
		if(!v.empty())
		{
			s = v.back();
			v.pop_back();
		}
		else
			s = new Str;
		return s;
	}

	inline void Free(Str* s)
	{
		v.push_back(s);
	}
} StrPool;

union VarValue
{
	int i;
	float f;
	Str* str;
	bool b;
	vector<VarValue>* arr;
};

struct Var
{
	VarValue v;
	VarType type, subtype;
	int offset;

	Var() {}
	Var(int value) : type(INT)
	{
		v.i = value;
	}
	Var(float f) : type(FLOAT)
	{
		v.f = f;
	}
	Var(string& s) : type(STR)
	{
		v.str = StrPool.Get();
		v.str->refs = 1;
		v.str->s = s;
	}
	Var(Str* s) : type(STR)
	{
		v.str = s;
	}

	void Clean()
	{
		if(type == STR && --v.str->refs == 0)
			StrPool.Free(v.str);
	}
};

vector<Var> vars;
vector<Var> stack;
//int args_offset, locals_offset;

struct FunctionInfo
{
	cstring name;
	void(*ptr)();
	VarType result;
	VarType params[4];
	uint params_count;
};

vector<FunctionInfo> functions;

void f_print()
{
	printf(stack.back().v.str->s.c_str());
	stack.back().Clean();
	stack.pop_back();
}

void f_getint()
{
	int a;
	scanf_s("%d", &a);
	stack.push_back(Var(a));
}

void f_getfloat()
{
	float a;
	scanf_s("%f", &a);
	stack.push_back(Var(a));
}

void f_getstring()
{
	Str* s = StrPool.Get();
	std::getline(std::cin, s->s);
	s->refs = 1;
	stack.push_back(Var(s));
}

void f_pause()
{
	_getch();
}

void f_random()
{
	int right = stack.back().v.i;
	stack.pop_back();
	int left = stack.back().v.i;
	if(left == right)
		return;
	if(left > right)
		std::swap(left, right);
	stack.back().v.i = rand() % (right - left + 1) + left;
}

inline int get_int(byte*& c)
{
	int i = *(int*)c;
	c += 4;
	return i;
}

inline float get_float(byte*& c)
{
	float f = *(float*)c;
	c += 4;
	return f;
}

void run(byte* code, vector<Str*> strs)
{
	byte* c = code;
	while(true)
	{
		Op op = (Op)*c;
		++c;
		switch(op)
		{
		case add:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var left = stack.back();
				stack.pop_back();
				assert(left.type == right.type && (left.type == INT || left.type == FLOAT || left.type == STR));
				if(left.type == INT)
					stack.push_back(Var(left.v.i + right.v.i));
				else if(left.type == FLOAT)
					stack.push_back(Var(left.v.f + right.v.f));
				else
				{
					g_tmp_string2 = left.v.str->s + right.v.str->s;
					--right.v.str->refs;
					if(right.v.str->refs == 0)
					{
						right.v.str->s = g_tmp_string2;
						right.v.str->refs = 1;
						--left.v.str->refs;
						if(left.v.str->refs == 0)
							StrPool.Free(left.v.str);
						stack.push_back(right);
					}
					else
					{
						--left.v.str->refs;
						if(left.v.str->refs == 0)
						{
							left.v.str->s = g_tmp_string2;
							left.v.str->refs = 1;
							stack.push_back(left);
						}
						else
							stack.push_back(Var(g_tmp_string2));
					}
				}
			}
			break;
		case sub:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == right.type && (left.type == INT || left.type == FLOAT));
				if(left.type == INT)
					left.v.i -= right.v.i;
				else
					left.v.f -= right.v.f;
			}
			break;
		case mul:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == right.type && (left.type == INT || left.type == FLOAT));
				if(left.type == INT)
					left.v.i *= right.v.i;
				else
					left.v.f *= right.v.f;
			}
			break;
		case o_div:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == right.type && (left.type == INT || left.type == FLOAT));
				if(left.type == INT)
				{
					if(right.v.i == 0)
						throw "Division by zero!";
					left.v.i /= right.v.i;
				}
				else
				{
					if(right.v.f == 0)
						throw "Division by zero!";
					left.v.f /= right.v.f;
				}
			}
			break;
		case mod:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == right.type && left.type == INT);
				if(right.v.i == 0)
					throw "Division by zero!";
				left.v.i %= right.v.i;
			}
			break;
		case neg:
			{
				assert(!stack.empty());
				Var& a = stack.back();
				assert(a.type == INT || a.type == FLOAT);
				if(a.type == FLOAT)
					a.v.f = -a.v.f;
				else
					a.v.i = -a.v.i;
			}
			break;
		case inc_pre:
		case inc_post:
		case dec_pre:
		case dec_post:
			{
				assert(!stack.empty());
				Var& a = stack.back();
				assert(a.type == REF);
				Var& v = vars[a.v.i];
				assert(v.type == INT || v.type == FLOAT || v.type == ARRAY);
				VarType type;
				VarValue* vv;

				if(v.type == ARRAY)
				{
					assert((v.subtype == INT || v.subtype == FLOAT) && v.offset >= 0 && v.offset < (int)v.v.arr->size());
					vv = &v.v.arr->at(v.offset);
					type = v.subtype;
				}
				else
				{
					vv = &v.v;
					type = v.type;
				}

				bool pre = (op == inc_pre || op == dec_pre);
				bool inc = (op == inc_pre || op == inc_post);

				if(!pre)
				{
					if(type == INT)
						a.v.i = vv->i;
					else
						a.v.f = vv->f;
				}

				if(inc)
				{
					if(type == INT)
						vv->i++;
					else
						vv->f++;
				}
				else
				{
					if(type == INT)
						vv->i--;
					else
						vv->f--;
				}

				if(pre)
				{
					if(type == INT)
						a.v.i = vv->i;
					else
						a.v.f = vv->f;
				}

				a.type = type;
			}
			break;
		case inc_pre:
			{
				assert(!stack.empty());
				Var& a = stack.back();
				assert(a.type == REF);
				Var& v = vars[a.v.i];
				assert(v.type == INT || v.type == FLOAT || v.type == ARRAY);
				if(v.type == ARRAY)
				{
					assert((v.subtype == INT || v.subtype == FLOAT) && v.offset >= 0 && v.offset < (int)v.v.arr->size());
					VarValue& vv = v.v.arr->at(v.offset);
					if(v.subtype == INT)
					{
						vv.i++;
						a.v.i = vv.i;
					}
					else
					{
						vv.f++;
						a.v.f = vv.f;
					}
					a.type = v.subtype;
				}
				else
				{
					if(v.type == INT)
					{
						v.v.i++;
						a.v.i = v.v.i;
					}
					else
					{
						v.v.f++;
						a.v.f = v.v.f;
					}
					a.type = v.type;
				}
			}
			break;
		case equal:
		case not_equal:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == right.type);
				bool result;
				if(left.type == INT)
					result = (left.v.i == right.v.i);
				else if(left.type == FLOAT)
					result = (left.v.f == right.v.f);
				else if(left.type == BOOL)
					result = (left.v.b == right.v.b);
				else if(left.type == STR)
				{
					result = (left.v.str->s == right.v.str->s);
					left.Clean();
					right.Clean();
				}
				else
				{
					assert(0);
					result = false;
				}
				if(op == not_equal)
					result = !result;
				left.type = BOOL;
				left.v.b = result;
			}
			break;
		case greater:
		case greater_equal:
		case less:
		case less_equal:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == right.type && (left.type == INT || left.type == FLOAT));
				bool result = false;
				switch(op)
				{
				case greater:
					if(left.type == INT)
						result = (left.v.i > right.v.i);
					else
						result = (left.v.f > right.v.f);
					break;
				case greater_equal:
					if(left.type == INT)
						result = (left.v.i >= right.v.i);
					else
						result = (left.v.f >= right.v.f);
					break;
				case less:
					if(left.type == INT)
						result = (left.v.i < right.v.i);
					else
						result = (left.v.f < right.v.f);
					break;
				case less_equal:
					if(left.type == INT)
						result = (left.v.i <= right.v.i);
					else
						result = (left.v.f <= right.v.f);
					break;
				}
				left.type = BOOL;
				left.v.b = result;
			}
			break;
		case and:
		case or:
			{
				assert(stack.size() >= 2u);
				Var right = stack.back();
				stack.pop_back();
				Var& left = stack.back();
				assert(left.type == right.type && left.type == BOOL);
				bool result;
				if(op == and)
					result = (left.v.b && right.v.b);
				else
					result = (left.v.b || right.v.b);
				left.v.b = result;
				left.type = BOOL;
			}
			break;
		case cast:
			{
				byte type = *c++;
				assert(!stack.empty());
				Var& a = stack.back();
				switch(type)
				{
				case STR:
					if(a.type == INT)
					{
						cstring s = Format("%d", a.v.i);
						a.v.str = StrPool.Get();
						a.v.str->s = s;
					}
					else if(a.type == FLOAT)
					{
						cstring s = Format("%g", a.v.f);
						a.v.str = StrPool.Get();
						a.v.str->s = s;
					}
					else if(a.type == BOOL)
					{
						cstring s = (a.v.b ? "true" : "false");
						a.v.str = StrPool.Get();
						a.v.str->s = s;
					}
					else
						assert(0);
					a.type = STR;
					a.v.str->refs = 1;
					break;
				case INT:
					if(a.type == FLOAT)
					{
						a.v.i = (int)a.v.f;
						a.type = INT;
					}
					else
						assert(0);
					break;
				case FLOAT:
					if(a.type == INT)
					{
						a.v.f = (float)a.v.i;
						a.type = FLOAT;
					}
					else
						assert(0);
					break;
				case BOOL:
					if(a.type == INT)
						a.v.b = (a.v.i != 0);
					else if(a.type == FLOAT)
						a.v.i = (a.v.f != 0.f);
					else
						assert(0);
					a.type = BOOL;
					break;
				default:
					assert(0);
					break;
				}
			}
			break;
		case locals:
			{
				byte b = *c++;
				vars.resize(b);
			}
			break;
		case set_local:
			{
				byte b = *c++;
				assert(b < vars.size());
				vars[b].Clean();
				vars[b] = stack.back();
			}
			break;
		case set_local_index:
			{
				byte b = *c++;
				int index = get_int(c);
				assert(b < vars.size());
				assert(!stack.empty());
				Var& v = vars[b];
				assert(v.type == ARRAY);
				// ----------------=-=-=-=-=d-=sa-d=sa-d=sa
			}
			break;
		case set_local_indexvar:
		case push_local:
			{
				byte b = *c++;
				assert(b < vars.size());
				if(vars[b].type == STR)
					vars[b].v.str->refs++;
				stack.push_back(vars[b]);
			}
			break;
		case push_local_ref:
			{
				byte b = *c++;
				assert(b < vars.size());
				Var v;
				v.type = REF;
				v.v.i = b;
				stack.push_back(v);
			}
			break;
		case push_cstr:
			{
				byte b = *c++;
				assert(b < strs.size());
				strs[b]->refs++;
				Var v;
				v.type = STR;
				v.v.str = strs[b];
				stack.push_back(v);
			}
			break;
		case push_int:
			stack.push_back(Var(get_int(c)));
			break;
		case push_float:
			stack.push_back(Var(get_float(c)));
			break;
		case pop:
			assert(!stack.empty());
			stack.back().Clean();
			stack.pop_back();
			break;
		case call:
			{
				byte b = *c++;
				assert(b < functions.size());
				FunctionInfo& f = functions[b];
#ifndef NDEBUG
				assert(stack.size() >= f.params_count);
				int i = 0;
				for(vector<Var>::reverse_iterator rit = stack.rbegin(), rend = stack.rend(); rit != rend; ++rit)
				{
					if(f.params_count == i)
						break;
					assert(rit->type == f.params[i]);
					++i;					
				}
#endif
				f.ptr();
			}
			break;
		case jmp:
			{
				int pos = get_int(c);
				c = code + pos;
			}
			break;
		case jmp_if:
			{
				int pos = get_int(c);
				assert(!stack.empty());
				Var& a = stack.back();
				assert(a.type == BOOL);
				if(a.v.i == 0)
					c = code + pos;
				stack.pop_back();
			}
			break;
		case ret:
			assert(stack.empty());
			return;
		}
	}
}

/*
type varname = call getint
type varname = var op var

statement
	vartype name = expr
	var = expr
	if (expr) block [else block]
	expr
	while (expr) block

block
	{ statement [statement ...] }
	statement

expr
	item [op item]

item
	func(args)
	var
	string
	int
*/
struct ParseVar
{
	string name;
	VarType type, subtype;
	int size;	
};

enum Operation
{
	OP_NONE,
	OP_ASSIGN,
	OP_EQUAL,
	OP_NOT_EQUAL,
	OP_GREATER,
	OP_GREATER_EQUAL,
	OP_LESS,
	OP_LESS_EQUAL,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_AND,
	OP_OR,
};

struct ParseNode
{
	enum Type
	{
		Var,
		Func,
		Op,
		Op2,
		OpRef,
		Cast,
		Str,
		Int,
		Float,
		List,
		If,
		While,
		Break
	};

	Type type;
	VarType result;
	int a;
	union
	{
		int b;
		float f;
	};
	vector<ParseNode*> nodes;

	inline void push(ParseNode* node)
	{
		nodes.push_back(node);
	}
};

enum Keyword
{
	IF,
	ELSE,
	WHILE,
	BREAK
};

enum Group
{
	G_VAR,
	G_FUNC,
	G_KEY
};

Tokenizer t(Tokenizer::F_UNESCAPE);
vector<byte> bcode;
vector<ParseVar> pvars;
vector<Str*> pstr;
bool in_loop;

ParseNode* Cast(ParseNode* node, VarType type)
{
	if(node->result == type)
		return node;

	bool can_cast = false;

	switch(type)
	{
	case INT:
		if(node->result == FLOAT)
			can_cast = true;
		break;
	case FLOAT:
		if(node->result == INT)
			can_cast = true;
		break;
	case STR:
		if(node->result == INT || node->result == BOOL || node->result == FLOAT)
			can_cast = true;
		break;
	case BOOL:
		if(node->result == INT)
			can_cast = true;
		break;
	case VOID:
		{
			ParseNode* tv = new ParseNode;
			tv->type = ParseNode::List;
			tv->result = VOID;
			tv->a = 1;
			tv->push(node);
			return tv;
		}
	default:
		assert(0);
		break;
	}

	if(can_cast)
	{
		ParseNode* ts = new ParseNode;
		ts->type = ParseNode::Cast;
		ts->a = type;
		ts->result = type;
		ts->push(node);
		return ts;
	}
	else
	{
		t.Throw(Format("Can't cast %s to %s.", VarTypeToString(node->result), VarTypeToString(type)));
		return NULL;
	}	
}

ParseNode* parse_expr(char closing=0);

bool peek_item()
{
	if(t.IsKeywordGroup(G_FUNC) || t.IsString() || t.IsInt() || t.IsFloat())
		return true;
	else if(t.IsItem())
	{
		// is var?
		const string& s = t.MustGetItem();
		for(ParseVar& pv : pvars)
		{
			if(pv.name == s)
				return true;
		}
	}
	return false;
}

ParseNode* parse_item()
{
	ParseNode* node = NULL;

	if(t.IsKeywordGroup(G_FUNC))
	{
		// func
		int func_id = t.GetKeywordId();
		FunctionInfo& f = functions[func_id];
		node = new ParseNode;
		node->type = ParseNode::Func;
		node->a = func_id;
		node->result = f.result;
		t.Next();
		// (
		t.AssertSymbol('(');
		t.Next();
		// args
		for(uint i = 0; i < f.params_count; ++i)
		{
			char c = ((i != f.params_count - 1) ? ',' : ')');
			node->push(Cast(parse_expr(c), f.params[i]));
			if(c == ',')
			{
				t.AssertSymbol(',');
				t.Next();
			}
		}
		// )
		t.AssertSymbol(')');
		t.Next();
	}
	else if(t.IsString())
	{
		Str* s = StrPool.Get();
		s->s = t.GetString();
		s->refs = 1;
		pstr.push_back(s);
		node = new ParseNode;
		node->type = ParseNode::Str;
		node->a = pstr.size() - 1;
		node->result = STR;
		t.Next();
	}
	else if(t.IsInt())
	{
		node = new ParseNode;
		node->type = ParseNode::Int;
		node->result = INT;
		node->a = t.GetInt();
		t.Next();
	}
	else if(t.IsFloat())
	{
		node = new ParseNode;
		node->type = ParseNode::Float;
		node->result = FLOAT;
		node->f = t.GetFloat();
		t.Next();
	}
	else if(t.IsItem())
	{
		// is var?
		const string& s = t.MustGetItem();
		int index = 0;
		bool found = false;
		for(ParseVar& pv : pvars)
		{
			if(pv.name == s)
			{
				found = true;
				break;
			}
			else
				++index;
		}

		if(found)
		{
			node = new ParseNode;
			node->type = ParseNode::Var;
			node->a = index;
			node->result = pvars[index].type;
			t.Next();
		}
	}

	return node;
}

enum Symbol
{
	S_LPAR,
	S_RPAR,
	S_ASSIGN,
	S_ASSIGN_ADD,
	S_ASSIGN_SUB,
	S_ASSIGN_MUL,
	S_ASSIGN_DIV,
	S_ASSIGN_MOD,
	S_EQUAL,
	S_NOT_EQUAL,
	S_GREATER,
	S_GREATER_EQUAL,
	S_LESS,
	S_LESS_EQUAL,
	S_ADD,
	S_SUB,
	S_MUL,
	S_DIV,
	S_MOD,
	S_INC_PRE,
	S_INC_POST,
	S_DEC_PRE,
	S_DEC_POST,
	S_UNARY_PLUS,
	S_UNARY_MINUS,
	S_AND,
	S_OR,
	//S_ARRAY_ITEM,
	S_OTHER
};

struct SymbolInfo
{
	int op;
	cstring name;
	int priority;
	bool right_to_left;
	bool single;
};

// http://en.cppreference.com/w/cpp/language/operator_precedence
SymbolInfo symbol_info[] = {
	OP_NONE, "lpar", -99, false, false,
	OP_NONE, "rpar", -99, false, false,
	OP_ASSIGN, "assign", 0, true, false,
	OP_ADD, "assign add", 0, true, false,
	OP_SUB, "assign sub", 0, true, false,
	OP_MUL, "assign mul", 0, true, false,
	OP_DIV, "assign div", 0, true, false,
	OP_MOD, "assign mod", 0, true, false,
	OP_EQUAL, "equal", 6, false, false,
	OP_NOT_EQUAL, "not equal", 6, false, false,
	OP_GREATER, "greater", 7, false, false,
	OP_GREATER_EQUAL, "greater equal", 7, false, false,
	OP_LESS, "less", 7, false, false,
	OP_LESS_EQUAL, "less equal", 7, false, false,
	OP_ADD, "add", 9, false, false,
	OP_SUB, "sub", 9, false, false,
	OP_MUL, "mul", 10, false, false,
	OP_DIV, "div", 10, false, false,
	OP_MOD, "mod", 10, false, false,
	inc_pre, "inc pre", 11, true, true,
	inc_post, "inc post", 12, false, true,
	dec_pre, "dec pre", 11, true, true,
	dec_post, "dec post", 12, false, true,
	OP_NONE, "unary plus", 11, true, true,
	OP_NONE, "unary minus", 11, true, true,
	OP_AND, "and", 2, false, false,
	OP_OR, "or", 1, false, false,
	//OP_NONE, "array get item", 12, false, true,
	OP_NONE, "other", 99, false, false,
};

enum OnLeft
{
	LEFT_NONE,
	LEFT_SYMBOL,
	LEFT_ITEM
};

struct ItemOrSymbol
{
	union
	{
		ParseNode* item;
		Symbol symbol;
	};
	bool is_symbol;

	ItemOrSymbol(ParseNode* item) : item(item), is_symbol(false) {}
	ItemOrSymbol(Symbol symbol) : symbol(symbol), is_symbol(true) {}
};

/*
== str int bool str/int str/bool int/bool
!= ^^^
> int (float)
>= int
< int
<= int
+ str, int
- int
* int
/ int
% int
*/
bool CanOp(Operation op, VarType left, VarType right, VarType& cast, VarType& result)
{
	switch(op)
	{
	case OP_EQUAL:
	case OP_NOT_EQUAL:
		result = BOOL;
		if(left == STR || right == STR)
		{
			cast = (left == right ? VOID : STR);			
			return true;
		}
		else if(left == FLOAT || right == FLOAT)
		{
			cast = (left == right ? VOID : FLOAT);
			return true;
		}
		else if(left == INT || right == INT)
		{
			cast = (left == right ? VOID : INT);
			return true;
		}
		else
		{
			cast = VOID;
			return true;
		}
	case OP_GREATER:
	case OP_GREATER_EQUAL:
	case OP_LESS:
	case OP_LESS_EQUAL:
		result = BOOL;
		if((left == FLOAT || left == INT) && (right == FLOAT || right == INT))
		{
			if(left == FLOAT || right == FLOAT)
			{
				cast = (left == right ? VOID : FLOAT);
				return true;
			}
			else
			{
				cast = VOID;
				return true;
			}
		}
		else
			return false;
	case OP_SUB:
	case OP_MUL:
	case OP_DIV:
	case OP_MOD:
		if((left == FLOAT || left == INT) && (right == FLOAT || right == INT))
		{
			if(left == FLOAT || right == FLOAT)
			{
				cast = (left == right ? VOID : FLOAT);
				result = FLOAT;
				return true;
			}
			else
			{
				cast = VOID;
				result = INT;
				return true;
			}
		}
		else
			return false;
	case OP_ADD:
		if(left == STR || right == STR)
		{
			result = STR;
			cast = (left == right ? VOID : STR);
			return true;
		}
		else if(left == FLOAT || right == FLOAT)
		{
			result = FLOAT;
			cast = (left == right ? VOID : FLOAT);
			return true;
		}
		else if(left == INT || right == INT)
		{
			result = INT;
			cast = (left == right ? VOID : INT);
			return true;
		}
		else
			return false;
	case OP_AND:
	case OP_OR:
		if((left == BOOL || left == INT || left == FLOAT) && (right == BOOL || right == INT || right == BOOL))
		{
			result = BOOL;
			cast = ((left == right && left == BOOL) ? VOID : BOOL);
			return true;
		}
		else
			return false;
	default:
		return false;
	}
}

ParseNode* parse_expr(char closing)
{
	vector<ItemOrSymbol> rpn_exit;
	vector<Symbol> rpn_stack;

	//-------------------------------------------
	// convert infix to reverse polish notation
	OnLeft left = LEFT_NONE;
	int level = 0;

	while(true)
	{
		if(peek_item())
		{
			if(left == LEFT_NONE || left == LEFT_SYMBOL)
			{
				rpn_exit.push_back(ItemOrSymbol(parse_item()));
				left = LEFT_ITEM;
			}
			else
				break;
		}
		else
		{
			if(t.IsSymbol())
			{
				Symbol s;
				switch(t.GetSymbol())
				{
				case '=':
					if(t.PeekSymbol('='))
						s = S_EQUAL;
					else
						s = S_ASSIGN;
					break;
				case '!':
					if(t.PeekSymbol('='))
						s = S_NOT_EQUAL;
					else
						s = S_OTHER;
					break;
				case '>':
					if(t.PeekSymbol('='))
						s = S_GREATER_EQUAL;
					else
						s = S_GREATER;
					break;
				case '<':
					if(t.PeekSymbol('='))
						s = S_LESS_EQUAL;
					else
						s = S_LESS;
					break;
				case '+':
					if(t.PeekSymbol('+'))
					{
						if(left == LEFT_NONE || left == LEFT_SYMBOL)
							s = S_INC_PRE;
						else
							s = S_INC_POST;
					}
					else if(t.PeekSymbol('='))
						s = S_ASSIGN_ADD;
					else if(left == LEFT_NONE || left == LEFT_SYMBOL)
						s = S_UNARY_PLUS;
					else
						s = S_ADD;
					break;
				case '-':
					if(t.PeekSymbol('-'))
					{
						if(left == LEFT_NONE || left == LEFT_SYMBOL)
							s = S_DEC_PRE;
						else
							s = S_DEC_POST;
					}
					else if(t.PeekSymbol('='))
						s = S_ASSIGN_SUB;
					else if(left == LEFT_NONE || left == LEFT_SYMBOL)
						s = S_UNARY_MINUS;
					else
						s = S_SUB;
					break;
				case '*':
					if(t.PeekSymbol('='))
						s = S_ASSIGN_MUL;
					else
						s = S_MUL;
					break;
				case '/':
					if(t.PeekSymbol('='))
						s = S_ASSIGN_DIV;
					else
						s = S_DIV;
					break;
				case '%':
					if(t.PeekSymbol('='))
						s = S_ASSIGN_MOD;
					else
						s = S_MOD;
					break;
				case '(':
					s = S_LPAR;
					break;
				case ')':
					s = S_RPAR;
					break;
				case '&':
					if(t.PeekSymbol('&'))
						s = S_AND;
					else
						s = S_OTHER;
					break;
				case '|':
					if(t.PeekSymbol('|'))
						s = S_OR;
					else
						s = S_OTHER;
					break;
				//case '[':
				//	s = S_ARRAY_ITEM;
				//	break;
				default:
					s = S_OTHER;
					break;
				}

				if(s == S_OTHER)
					break;
				else if(s == S_LPAR)
				{
					rpn_stack.push_back(s);
					left = LEFT_NONE;
					++level;
				}
				else if(s == S_RPAR)
				{
					if(level == 0 && closing == ')')
						break;
					--level;
					while(true)
					{
						Symbol s2 = rpn_stack.back();
						rpn_stack.pop_back();
						if(s2 == S_LPAR)
						{
							left = LEFT_ITEM;
							break;
						}
						else
						{
							rpn_exit.push_back(ItemOrSymbol(s2));
							if(rpn_stack.empty())
								t.Throw("Invalid closing parenthesis.");
						}
					}
				}
				/*else if(s == S_ARRAY_ITEM)
				{
					if(left != LEFT_ITEM)
						t.Unexpected();
					ParseNode* nl = rpn_exit.back().item;
					if(nl->type != ParseNode::Var || pvars[nl->a].type != ARRAY)
						t.Unexpected();
					t.Next();
					ParseNode* expr = Cast(parse_expr(), INT);
					// validate if const
					if(expr->type == ParseNode::Int)
					{
						if(expr->a >= pvars[nl->a].size)
						{
							ParseVar& v = pvars[nl->a];
							t.Throw(Format("Index %d is out of bounds for %s[%d] %s.", expr->a, VarTypeToString(v.subtype),
								v.size, v.name.c_str()));
						}
					}
					
				}*/
				else
				{
					if(left != LEFT_ITEM && s != S_UNARY_PLUS && s != S_UNARY_MINUS && s != S_INC_PRE && s != S_DEC_PRE)
						t.Unexpected();
					while(!rpn_stack.empty())
					{
						Symbol s2 = rpn_stack.back();
						SymbolInfo& s_info = symbol_info[s];
						SymbolInfo& s2_info = symbol_info[s2];
						bool ok = false;
						if(!s_info.right_to_left)
						{
							if(s_info.priority <= s2_info.priority)
								ok = true;
						}
						else
						{
							if(s_info.priority < s2_info.priority)
								ok = true;
						}
						if(ok)
						{
							rpn_exit.push_back(ItemOrSymbol(s2));
							rpn_stack.pop_back();
						}
						else
							break;
					}
					rpn_stack.push_back(s);
					left = LEFT_SYMBOL;
				}

				t.Next();
			}
			else
				break;
		}		
	}

	if(left == LEFT_SYMBOL)
	{
		Symbol s = rpn_stack.back();
		if(s != S_INC_POST && s != S_DEC_POST)
			t.Unexpected();
	}

	while(!rpn_stack.empty())
	{
		Symbol s = rpn_stack.back();
		if(s == S_LPAR)
			t.Throw("Missing closing parenthesis.");
		rpn_exit.push_back(ItemOrSymbol(s));
		rpn_stack.pop_back();
	}

	if(rpn_exit.empty())
		t.Unexpected();

	//-------------------------------------------
	// convert reverse polish notation to parse nodes tree
	vector<ParseNode*> nodes;
	for(ItemOrSymbol& item : rpn_exit)
	{
		if(!item.is_symbol)
		{
			nodes.push_back(item.item);
			continue;
		}

		SymbolInfo& info = symbol_info[item.symbol];

		if(info.single)
		{
			if(item.symbol == S_UNARY_PLUS || item.symbol == S_UNARY_MINUS)
			{
				if(nodes.empty())
					t.Throw("Failed to parse expression tree for unary operator.");
				ParseNode* left = nodes.back();
				if(left->result != INT && left->result != FLOAT)
					t.Throw(Format("Can't %s type %s.", info.name, VarTypeToString(left->result)));
				if(item.symbol == S_UNARY_MINUS)
				{
					ParseNode* node = new ParseNode;
					node->type = ParseNode::Op2;
					node->a = neg;
					node->result = left->result;
					node->push(left);
					nodes.pop_back();
					nodes.push_back(node);
				}
			}
			else
			{
				ParseNode* left = nodes.back();
				nodes.pop_back();
				if(left->type != ParseNode::Var)
					t.Throw(Format("Can't %s, left must be var, got %s.", info.name, VarTypeToString(left->result)));
				if(left->result != INT && left->result != FLOAT)
					t.Throw(Format("Can't %s type %s.", info.name, VarTypeToString(left->result)));
				ParseNode* node = new ParseNode;
				node->type = ParseNode::OpRef;
				node->a = info.op;
				node->b = left->a;
				node->result = left->result;
				delete left;
				nodes.push_back(node);
			}
		}
		else
		{
			if(nodes.size() < 2u)
				t.Throw("Failed to parse expression tree.");
			ParseNode* node = new ParseNode;
			node->type = ParseNode::Op;
			ParseNode* right = nodes.back();
			nodes.pop_back();
			ParseNode* left = nodes.back();
			nodes.pop_back();
			
			if(left->result == VOID || right->result == VOID)
				t.Throw(Format("Can't %s types %s and %s.", info.name, VarTypeToString(left->result), VarTypeToString(right->result)));

			if(item.symbol == S_ASSIGN)
			{
				if(left->type != ParseNode::Var)
					t.Throw(Format("Can't %s, left is not variable.", info.name));
				node->a = OP_ASSIGN;
				node->b = left->a;
				node->result = left->result;
				node->push(Cast(right, left->result));
				delete left;
			}
			else if(item.symbol >= S_ASSIGN_ADD && item.symbol <= S_ASSIGN_MOD)
			{
				if(left->type != ParseNode::Var)
					t.Throw(Format("Can't %s, left is not variable.", info.name));
				VarType cast, result;
				bool can_op = CanOp((Operation)info.op, left->result, right->result, cast, result);
				if(!can_op || result != left->result)
					t.Throw(Format("Can't %s types %s and %s.", info.name, VarTypeToString(left->result), VarTypeToString(right->result)));

				ParseNode* node_op = new ParseNode;
				node_op->type = ParseNode::Op;
				node_op->a = info.op;
				node_op->result = left->result;
				node_op->push(left);
				node_op->push(right);

				node->a = OP_ASSIGN;
				node->b = left->a;
				node->result = left->result;
				node->push(node_op);
			}
			else
			{
				VarType cast, result;
				bool can_op = CanOp((Operation)info.op, left->result, right->result, cast, result);
				if(!can_op)
					t.Throw(Format("Can't %s types %s and %s.", info.name, VarTypeToString(left->result), VarTypeToString(right->result)));
				if(cast != VOID)
				{
					left = Cast(left, cast);
					right = Cast(right, cast);
				}
				node->a = info.op;
				node->result = result;
				node->push(left);
				node->push(right);
			}

			nodes.push_back(node);
		}
	}

	if(nodes.size() != 1u)
		t.Throw("Broken expression tree.");

	return nodes[0];
}

ParseNode* parse_line();

// block must return void
ParseNode* parse_block()
{
	if(t.IsSymbol('{'))
	{
		t.Next();
		ParseNode* node = new ParseNode;
		node->type = ParseNode::List;
		node->result = VOID;
		node->a = 0;

		while(true)
		{
			if(t.IsSymbol('}'))
				break;
			node->push(Cast(parse_line(), VOID));
		}

		t.Next();
		return node;
	}
	else
		return Cast(parse_line(), VOID);
}

ParseNode* parse_line()
{
	if(t.IsKeywordGroup(G_VAR))
	{
		// var
		VarType type = (VarType)t.GetKeywordId();
		//VarType subtype = VOID;
		//int size = 0;
		t.Next();

		// is array?
		/*if(t.IsSymbol('['))
		{
			subtype = type;
			type = ARRAY;
			t.Next();
			size = t.MustGetInt();
			if(size <= 0)
				t.Throw(Format("Invalid array size %d.", size));
			t.Next();
			t.AssertSymbol(']');
			t.Next();
		}*/

		ParseNode* list = new ParseNode;
		list->type = ParseNode::List;
		list->result = VOID;
		list->a = 0;

		while(true)
		{
			// name
			const string& var_name = t.MustGetItem();
			for(ParseVar& pv : pvars)
			{
				if(pv.name == var_name)
					t.Throw(Format("Var %s already declared.", var_name.c_str()));
			}
			ParseVar& pv = Add1(pvars);
			pv.name = var_name;
			pv.type = type;
			//pv.subtype = subtype;
			//pv.size = size;
			t.Next();

			// = or ,
			if(t.IsSymbol('='))
			{
				t.Next();

				// expr
				ParseNode* rnode = Cast(parse_expr(), type);

				ParseNode* node = new ParseNode;
				node->type = ParseNode::Op;
				node->a = OP_ASSIGN;
				node->b = pvars.size() - 1;
				node->result = type;
				node->push(rnode);

				list->push(Cast(node, VOID));
			}
			
			if(t.IsSymbol(','))
				t.Next();
			else
				break;
		}
		
		// return result
		if(list->nodes.size() == 1)
		{
			ParseNode* node = list->nodes[0];
			delete list;
			return node;
		}
		else
			return list;
	}
	else if(t.IsKeywordGroup(G_KEY))
	{
		if(t.IsKeyword(IF))
		{
			// if
			t.Next();

			// condition
			ParseNode* con = Cast(parse_expr(), BOOL);

			// expression
			ParseNode* expr = parse_block();

			ParseNode* node = new ParseNode;
			node->type = ParseNode::If;
			node->result = VOID;
			node->push(con);
			node->push(expr);

			// else
			if(t.IsKeyword(ELSE, 2))
			{
				t.Next();
				node->push(parse_block());
			}

			return node;
		}
		else if(t.IsKeyword(WHILE))
		{
			// while
			t.Next();
			
			// condition
			ParseNode* con = Cast(parse_expr(), BOOL);

			bool prev_in_loop = in_loop;
			in_loop = true;

			ParseNode* node = new ParseNode;
			node->type = ParseNode::While;
			node->result = VOID;
			node->push(con);
			node->push(parse_block());
			return node;
		}
		else if(t.IsKeyword(BREAK))
		{
			// break
			t.Next();

			if(!in_loop)
				t.Unexpected();

			ParseNode* node = new ParseNode;
			node->type = ParseNode::Break;
			node->result = VOID;
			return node;
		}
		else
		{
			t.Unexpected();
			return NULL;
		}
	}
	else
	{
		// is var?
		bool found = false;
		int index = 0;
		if(t.IsItem())
		{
			const string& s = t.GetItem();
			for(ParseVar& pv : pvars)
			{
				if(pv.name == s)
				{
					found = true;
					break;
				}
				else
					++index;
			}
		}

		if(found)
		{
			// var assignment
			ParseNode* node = parse_expr();
			if(node->type != ParseNode::Op || node->a != OP_ASSIGN)
				t.Throw(Format("Broken variable assignment %s.", pvars[index].name.c_str()));
			return Cast(node, VOID);
		}
		else
			return parse_expr();
	}
}

uint add_jump()
{
	uint pos = bcode.size();
	bcode.push_back(0);
	bcode.push_back(0);
	bcode.push_back(0);
	bcode.push_back(0);
	return pos;
}

void set_jump(uint pos)
{
	uint new_pos = bcode.size();
	*(uint*)&bcode[pos] = new_pos;
}

vector<uint>* jmp_pts;

void apply_node(ParseNode* node)
{
	if(node->type == ParseNode::If)
	{
		if(node->nodes.size() == 2u)
		{
			// if condition else jmp end
			//		code if true
			// end:
			apply_node(node->nodes[0]);
			bcode.push_back(jmp_if);
			uint end_pos = add_jump();
			apply_node(node->nodes[1]);
			set_jump(end_pos);
		}
		else
		{
			// if condition else jmp else
			//		code if true
			//		jmp end
			// else:
			//		code if else
			// end:
			apply_node(node->nodes[0]);
			bcode.push_back(jmp_if);
			uint else_pos = add_jump();
			apply_node(node->nodes[1]);
			bcode.push_back(jmp);
			uint end_pos = add_jump();
			set_jump(else_pos);
			apply_node(node->nodes[2]);
			set_jump(end_pos);
		}
		return;
	}
	else if(node->type == ParseNode::While)
	{
		uint start_pos = bcode.size();
		apply_node(node->nodes[0]);
		bcode.push_back(jmp_if);
		uint jmp_to_end = add_jump();
		vector<uint> new_jmp_pts;
		vector<uint>* old_jmp_pts = jmp_pts;
		jmp_pts = &new_jmp_pts;
		for(uint i = 1; i < node->nodes.size(); ++i)
			apply_node(node->nodes[i]);
		// add jump to start
		bcode.push_back(jmp);
		uint p = bcode.size();
		bcode.resize(bcode.size() + 4);
		*(uint*)&bcode[p] = start_pos;
		// end, set jumps to end
		set_jump(jmp_to_end);
		for(uint pp : new_jmp_pts)
			set_jump(pp);
		// restore jump points
		jmp_pts = old_jmp_pts;
		return;
	}

	for(ParseNode* n : node->nodes)
		apply_node(n);

	switch(node->type)
	{
	case ParseNode::Var:
		bcode.push_back(push_local);
		bcode.push_back(node->a);
		break;
	case ParseNode::Func:
		bcode.push_back(call);
		bcode.push_back(node->a);
		break;
	case ParseNode::Op:
		switch(node->a)
		{
		case OP_ASSIGN:
			bcode.push_back(set_local);
			bcode.push_back(node->b);
			break;
		case OP_EQUAL:
			bcode.push_back(equal);
			break;
		case OP_NOT_EQUAL:
			bcode.push_back(not_equal);
			break;
		case OP_GREATER:
			bcode.push_back(greater);
			break;
		case OP_GREATER_EQUAL:
			bcode.push_back(greater_equal);
			break;
		case OP_LESS:
			bcode.push_back(less);
			break;
		case OP_LESS_EQUAL:
			bcode.push_back(less_equal);
			break;
		case OP_ADD:
			bcode.push_back(add);
			break;
		case OP_SUB:
			bcode.push_back(sub);
			break;
		case OP_MUL:
			bcode.push_back(mul);
			break;
		case OP_DIV:
			bcode.push_back(o_div);
			break;
		case OP_MOD:
			bcode.push_back(mod);
			break;
		case OP_AND:
			bcode.push_back(and);
			break;
		case OP_OR:
			bcode.push_back(or);
			break;
		default:
			assert(0);
			break;
		}
		break;
	case ParseNode::Op2:
		bcode.push_back(node->a);
		break;
	case ParseNode::OpRef:
		bcode.push_back(push_local_ref);
		bcode.push_back(node->b);
		bcode.push_back(node->a);
		break;
	case ParseNode::Cast:
		bcode.push_back(cast);
		bcode.push_back(node->result);
		break;
	case ParseNode::Str:
		bcode.push_back(push_cstr);
		bcode.push_back(node->a);
		break;
	case ParseNode::Int:
		bcode.push_back(push_int);
		bcode.push_back(node->a & 0xFF);
		bcode.push_back((node->a & 0xFF00) >> 8);
		bcode.push_back((node->a & 0xFF0000) >> 16);
		bcode.push_back((node->a & 0xFF000000) >> 24);
		break;
	case ParseNode::Float:
		bcode.push_back(push_float);
		bcode.push_back(node->a & 0xFF);
		bcode.push_back((node->a & 0xFF00) >> 8);
		bcode.push_back((node->a & 0xFF0000) >> 16);
		bcode.push_back((node->a & 0xFF000000) >> 24);
		break;
	case ParseNode::List:
		if(node->a == 1)
			bcode.push_back(pop);
		break;
	case ParseNode::Break:
		assert(jmp_pts);
		bcode.push_back(jmp);
		jmp_pts->push_back(add_jump());
		break;
	default:
		assert(0);
		break;
	}
}

bool parse(cstring filename)
{
	t.AddKeyword("int", INT, G_VAR);
	t.AddKeyword("string", STR, G_VAR);
	t.AddKeyword("bool", BOOL, G_VAR);
	t.AddKeyword("float", FLOAT, G_VAR);

	int i = 0;
	for(FunctionInfo& fi : functions)
	{
		t.AddKeyword(fi.name, i, G_FUNC);
		++i;
	}

	t.AddKeyword("if", IF, G_KEY);
	t.AddKeyword("else", ELSE, G_KEY);
	t.AddKeyword("while", WHILE, G_KEY);
	t.AddKeyword("break", BREAK, G_KEY);

	t.FromFile(filename);

	vector<ParseNode*> nodes;

	try
	{
		t.Next();
		while(!t.IsEof())
			nodes.push_back(parse_line());
	}
	catch(cstring err)
	{
		printf("PARSE ERROR: %s\n\n(ok)", err);
		_getch();
		return false;
	}

	if(!pvars.empty())
	{
		bcode.push_back(locals);
		bcode.push_back(pvars.size());
	}

	for(ParseNode* n : nodes)
	{
		apply_node(n);
		if(n->result != VOID)
			bcode.push_back(pop);
	}

	bcode.push_back(ret);

	return true;
}

void register_functions()
{
	{
		FunctionInfo& f = Add1(functions);
		f.name = "print";
		f.ptr = f_print;
		f.result = VOID;
		f.params[0] = STR;
		f.params_count = 1;
	}
	{
		FunctionInfo& f = Add1(functions);
		f.name = "getint";
		f.ptr = f_getint;
		f.result = INT;
		f.params_count = 0;
	}
	{
		FunctionInfo& f = Add1(functions);
		f.name = "getfloat";
		f.ptr = f_getfloat;
		f.result = FLOAT;
		f.params_count = 0;
	}
	{
		FunctionInfo& f = Add1(functions);
		f.name = "getstr";
		f.ptr = f_print;
		f.result = STR;
		f.params_count = 0;
	}
	{
		FunctionInfo& f = Add1(functions);
		f.name = "pause";
		f.ptr = f_pause;
		f.result = VOID;
		f.params_count = 0;
	}
	{
		FunctionInfo& f = Add1(functions);
		f.name = "random";
		f.ptr = f_random;
		f.result = INT;
		f.params[0] = INT;
		f.params[1] = INT;
		f.params_count = 2;
	}
}

int main()
{
	srand((uint)time(0));
	register_functions();
	if(!parse("script/5.txt"))
		return 1;
	try
	{
		run(&bcode[0], pstr);
	}
	catch(cstring err)
	{
		printf("RUN ERROR: %s", err);
		_getch();
		return 2;
	}

	return 0;
}

/*
+ strict mode: if() while(), warn about cast ?
+ functions
+ object
+ vaarg func params<any>
+ object -> c++ class
+ any -> int/float/object (have type info)
+ do { } while
+ for
+ array
+ dynamic array size {int a= getint(); int[a] arr;}
+ array initialization {int[3] arr = {0,1,2};}
+ vector
+ map
+ list
+ bitwise op: & | ^ >> << &= |= ^= >>= <<= ~
+ string.length print("test".length)
+ c++ functions register

OPTIMIZATION:
+ const cast (while 1) - cast to bool
+ optimize const op (3+4) (1==3)
+ optimize const if (if 1==1)
+ const while (while 1)
+ reuse stack variables
+ set_local, push_local
*/
