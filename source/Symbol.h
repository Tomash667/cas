#pragma once

enum SYMBOL
{
	S_NONE,
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
	S_SET_REF,
	S_SET_LONG_REF,
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
	BS_SET_REF, // ->
	BS_SET_LONG_REF, // -->
	BS_MAX
};

struct ParseNode;

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

	SymbolNode(ParseNode* node) : node(node), is_symbol(false) {}
	SymbolNode(SYMBOL symbol, ParseNode* node = nullptr) : symbol(symbol), node(node), is_symbol(true) {}
};

struct BasicSymbolInfo
{
	BASIC_SYMBOL symbol;
	cstring text;
	SYMBOL pre_symbol;
	SYMBOL post_symbol;
	SYMBOL op_symbol;
	cstring full_over_text;

	SYMBOL operator [](int index)
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

	cstring GetOverloadText()
	{
		return full_over_text ? full_over_text : text;
	}
};

extern SymbolInfo symbols[];
extern BasicSymbolInfo basic_symbols[];
