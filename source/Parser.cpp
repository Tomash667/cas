#include "Pch.h"
#include "CasImpl.h"
#include "Parser.h"
#include "ParserImpl.h"
#include "Module.h"

#define COMBINE(x,y) (((x)&0xFF)|(((y)&0xFF)<<8))

// http://en.cppreference.com/w/cpp/language/operator_precedence
SymbolInfo symbols[] = {
	S_ADD, "add", 6, true, 2, ADD, ST_NONE, "$opAdd", "+",
	S_SUB, "subtract", 6, true, 2, SUB, ST_NONE, "$opSub", "-",
	S_MUL, "multiply", 5, true, 2, MUL, ST_NONE, "$opMul", "*",
	S_DIV, "divide", 5, true, 2, DIV, ST_NONE, "$opDiv", "/",
	S_MOD, "modulo", 5, true, 2, MOD, ST_NONE, "$opMod", "%",
	S_BIT_AND, "bit and", 10, true, 2, BIT_AND, ST_NONE, "$opBitAnd", "&",
	S_BIT_OR, "bit or", 12, true, 2, BIT_OR, ST_NONE, "$opBitOr", "|",
	S_BIT_XOR, "bit xor", 11, true, 2, BIT_XOR, ST_NONE, "$opBitXor", "^",
	S_BIT_LSHIFT, "bit left shift", 7, true, 2, BIT_LSHIFT, ST_NONE, "$opBitLshift", "<<",
	S_BIT_RSHIFT, "bit right shift", 7, true, 2, BIT_RSHIFT, ST_NONE, "$opBitRshift", ">>",
	S_PLUS, "unary plus", 3, false, 1, NOP, ST_NONE, "$opPlus", "+",
	S_MINUS, "unary minus", 3, false, 1, NEG, ST_NONE, "$opMinus", "-",
	S_EQUAL, "equal", 9, true, 2, EQ, ST_NONE, "$opEqual", "==",
	S_NOT_EQUAL, "not equal", 9, true, 2, NOT_EQ, ST_NONE, "$opNotEqual", "!=",
	S_GREATER, "greater", 8, true, 2, GR, ST_NONE, "$opGreater", ">",
	S_GREATER_EQUAL, "greater equal", 8, true, 2, GR_EQ, ST_NONE, "$opGreaterEqual", ">=",
	S_LESS, "less", 8, true, 2, LE, ST_NONE, "$opLess", "<",
	S_LESS_EQUAL, "less equal", 8, true, 2, LE_EQ, ST_NONE, "$opLessEqual", "<=",
	S_AND, "and", 13, true, 2, AND, ST_NONE, nullptr, "&&",
	S_OR, "or", 14, true, 2, OR, ST_NONE, nullptr, "||",
	S_NOT, "not", 3, false, 1, NOT, ST_NONE,  "$opNot", "!",
	S_BIT_NOT, "bit not", 3, false, 1, BIT_NOT, ST_NONE, "$opBitNot", "~",
	S_MEMBER_ACCESS, "member access", 2, true, 2, NOP, ST_NONE, nullptr, ".",
	S_ASSIGN, "assign", 15, false, 2, NOP, ST_ASSIGN, "$opAssign", "=",
	S_ASSIGN_ADD, "assign add", 15, false, 2, S_ADD, ST_ASSIGN, "$opAssignAdd", "+=",
	S_ASSIGN_SUB, "assign subtract", 15, false, 2, S_SUB, ST_ASSIGN, "$opAssignSub", "-=",
	S_ASSIGN_MUL, "assign multiply", 15, false, 2, S_MUL, ST_ASSIGN, "$opAssignMul", "*=",
	S_ASSIGN_DIV, "assign divide", 15, false, 2, S_DIV, ST_ASSIGN, "$opAssignDiv", "/=",
	S_ASSIGN_MOD, "assign modulo", 15, false, 2, S_MOD, ST_ASSIGN, "$opAssignMod", "%=",
	S_ASSIGN_BIT_AND, "assign bit and", 15, false, 2, S_BIT_AND, ST_ASSIGN, "$opAssignBitAnd", "&=",
	S_ASSIGN_BIT_OR, "assign bit or", 15, false, 2, S_BIT_OR, ST_ASSIGN, "$opAssignBitOr", "|=",
	S_ASSIGN_BIT_XOR, "assign bit xor", 15, false, 2, S_BIT_XOR, ST_ASSIGN, "$opAssignBitXor", "^=",
	S_ASSIGN_BIT_LSHIFT, "assign bit left shift", 15, false, 2, S_BIT_LSHIFT, ST_ASSIGN, "$opAssignBitLshift", "<<=",
	S_ASSIGN_BIT_RSHIFT, "assign bit right shift", 15, false, 2, S_BIT_RSHIFT, ST_ASSIGN, "$opAssignBitRshift", ">>=",
	S_PRE_INC, "pre increment", 3, false, 1, INC, ST_INC_DEC, "$opPreInc", "++",
	S_PRE_DEC, "pre decrement", 3, false, 1, DEC, ST_INC_DEC, "$opPreDec", "--",
	S_POST_INC, "post increment", 2, true, 1, INC, ST_INC_DEC, "$opPostInc", "++",
	S_POST_DEC, "post decrement", 2, true, 1, DEC, ST_INC_DEC, "$opPostDec", "--",
	S_IS, "reference equal", 8, true, 2, IS, ST_NONE, nullptr, "is",
	S_AS, "cast", 8, true, 2, CAST, ST_NONE, nullptr, "as",
	S_SUBSCRIPT, "subscript", 2, true, 1, NOP, ST_SUBSCRIPT, "$opIndex", "[]",
	S_CALL, "function call", 2, true, 1, NOP, ST_CALL, "$opCall", "()",
	S_TERNARY, "ternary", 15, false, 2, NOP, ST_NONE, nullptr, "?:",
	S_SET_REF, "assign reference", 15, false, 2, NOP, ST_NONE, nullptr, "->",
	S_SET_LONG_REF, "assign long reference", 15, false, 2, NOP, ST_NONE, nullptr, "-->",
	S_INVALID, "invalid", 99, true, 0, NOP, ST_NONE, nullptr, ""
};
static_assert(sizeof(symbols) / sizeof(SymbolInfo) == S_MAX, "Missing symbols.");

BasicSymbolInfo basic_symbols[] = {
	BS_INC, "+", S_PLUS, S_INVALID, S_ADD, nullptr,
	BS_DEC, "-", S_MINUS, S_INVALID, S_SUB,  nullptr,
	BS_MUL, "*", S_INVALID, S_INVALID, S_MUL, nullptr,
	BS_DIV, "/", S_INVALID, S_INVALID, S_DIV, nullptr,
	BS_MOD, "%", S_INVALID, S_INVALID, S_MOD, nullptr,
	BS_BIT_AND, "&", S_INVALID, S_INVALID, S_BIT_AND, nullptr,
	BS_BIT_OR, "|", S_INVALID, S_INVALID, S_BIT_OR, nullptr,
	BS_BIT_XOR, "^", S_INVALID, S_INVALID, S_BIT_XOR, nullptr,
	BS_BIT_LSHIFT, "<<", S_INVALID, S_INVALID, S_BIT_LSHIFT, nullptr,
	BS_BIT_RSHIFT, ">>", S_INVALID, S_INVALID, S_BIT_RSHIFT, nullptr,
	BS_EQUAL, "==", S_INVALID, S_INVALID, S_EQUAL, nullptr,
	BS_NOT_EQUAL, "!=", S_INVALID, S_INVALID, S_NOT_EQUAL, nullptr,
	BS_GREATER, ">", S_INVALID, S_INVALID, S_GREATER, nullptr,
	BS_GREATER_EQUAL, ">=", S_INVALID, S_INVALID, S_GREATER_EQUAL, nullptr,
	BS_LESS, "<", S_INVALID, S_INVALID, S_LESS, nullptr,
	BS_LESS_EQUAL, "<=", S_INVALID, S_INVALID, S_LESS_EQUAL, nullptr,
	BS_AND, "&&", S_INVALID, S_INVALID, S_AND, nullptr,
	BS_OR, "||", S_INVALID, S_INVALID, S_OR, nullptr,
	BS_NOT, "!", S_NOT, S_INVALID, S_INVALID, nullptr,
	BS_BIT_NOT, "~", S_BIT_NOT, S_INVALID, S_INVALID, nullptr,
	BS_ASSIGN, "=", S_INVALID, S_INVALID, S_ASSIGN, nullptr,
	BS_ASSIGN_ADD, "+=", S_INVALID, S_INVALID, S_ASSIGN_ADD, nullptr,
	BS_ASSIGN_SUB, "-=", S_INVALID, S_INVALID, S_ASSIGN_SUB, nullptr,
	BS_ASSIGN_MUL, "*=", S_INVALID, S_INVALID, S_ASSIGN_MUL, nullptr,
	BS_ASSIGN_DIV, "/=", S_INVALID, S_INVALID, S_ASSIGN_DIV, nullptr,
	BS_ASSIGN_MOD, "%=", S_INVALID, S_INVALID, S_ASSIGN_MOD, nullptr,
	BS_ASSIGN_BIT_AND, "&=", S_INVALID, S_INVALID, S_ASSIGN_BIT_AND, nullptr,
	BS_ASSIGN_BIT_OR, "|=", S_INVALID, S_INVALID, S_ASSIGN_BIT_OR, nullptr,
	BS_ASSIGN_BIT_XOR, "^=", S_INVALID, S_INVALID, S_ASSIGN_BIT_XOR, nullptr,
	BS_ASSIGN_BIT_LSHIFT, "<<=", S_INVALID, S_INVALID, S_ASSIGN_BIT_LSHIFT, nullptr,
	BS_ASSIGN_BIT_RSHIFT, ">>=", S_INVALID, S_INVALID, S_ASSIGN_BIT_RSHIFT, nullptr,
	BS_DOT, ".", S_INVALID, S_INVALID, S_MEMBER_ACCESS, nullptr,
	BS_INC, "++", S_PRE_INC, S_POST_INC, S_INVALID, nullptr,
	BS_DEC, "--", S_PRE_DEC, S_POST_DEC, S_INVALID, nullptr,
	BS_IS, "is", S_INVALID, S_INVALID, S_IS, nullptr,
	BS_AS, "as", S_INVALID, S_INVALID, S_AS, nullptr,
	BS_SUBSCRIPT, "[", S_INVALID, S_SUBSCRIPT, S_INVALID, "[]",
	BS_CALL, "(", S_INVALID, S_CALL, S_INVALID, "()",
	BS_TERNARY, "?", S_INVALID, S_INVALID, S_INVALID, nullptr,
	BS_SET_REF, "->", S_INVALID, S_INVALID, S_SET_REF, nullptr,
	BS_SET_LONG_REF, "-->", S_INVALID, S_INVALID, S_SET_LONG_REF, nullptr
};
static_assert(sizeof(basic_symbols) / sizeof(BasicSymbolInfo) == BS_MAX, "Missing basic symbols.");

Parser::Parser(Module* module) : module(module), t(Tokenizer::F_SEEK | Tokenizer::F_UNESCAPE | Tokenizer::F_CHAR | Tokenizer::F_HIDE_ID), empty_string(-1),
main_block(nullptr)
{
	AddKeywords();
	AddChildModulesKeywords();
}

Parser::~Parser()
{
	DeleteElements(ufuncs);
	if(main_block)
		main_block->Free();
}

bool Parser::VerifyTypeName(cstring type_name, int& type_index)
{
	assert(type_name);
	t.CheckItemOrKeyword(type_name);
	if(t.IsKeyword())
	{
		if(t.IsKeywordGroup(G_VAR))
			type_index = t.GetKeywordId(G_VAR);
		else
			type_index = -1;
		return false;
	}
	else
		return true;
}

void Parser::AddType(Type* type)
{
	assert(type);
	t.AddKeyword(type->name.c_str(), type->index, G_VAR);
}

void Parser::AddKeywords()
{
	// register keywords
	t.AddKeywords(G_KEYWORD, {
		{ "if", K_IF },
		{ "else", K_ELSE },
		{ "do", K_DO },
		{ "while", K_WHILE },
		{ "for", K_FOR },
		{ "break", K_BREAK },
		{ "return", K_RETURN },
		{ "class", K_CLASS },
		{ "struct", K_STRUCT },
		{ "is", K_IS },
		{ "as", K_AS },
		{ "operator", K_OPERATOR },
		{ "switch", K_SWITCH },
		{ "case", K_CASE },
		{ "default", K_DEFAULT },
		{ "implicit", K_IMPLICIT },
		{ "delete", K_DELETE },
		{ "static", K_STATIC },
		{ "enum", K_ENUM }
	}, "keywords");

	// const
	t.AddKeywords(G_CONST, {
		{ "true", C_TRUE },
		{ "false", C_FALSE },
		{ "this", C_THIS }
	}, "const");

	t.AddKeywordGroup("var", G_VAR);
}

void Parser::AddChildModulesKeywords()
{
	for(auto& m : module->modules)
	{
		if(m.first == module->index)
			continue;
		for(Type* type : m.second->types)
		{
			if(!type->IsHidden())
				AddType(type);
		}
	}
}

bool Parser::Parse(ParseSettings& settings)
{
	optimize = settings.optimize;
	ufunc_offset = module->ufuncs.size();
	tmp_type_offset = module->tmp_types.size();
	t.FromString(settings.input);

	try
	{
		ParseCode();
		Optimize();
		CheckReturnValues();
		CopyFunctionChangedStructs();
		ConvertToBytecode();
		FinishRunModule();
		Cleanup();

		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		Event(EventType::Error, e.ToString());
		Cleanup();

		return false;
	}
}

void Parser::FinishRunModule()
{
	// convert parse functions to script functions
	module->ufuncs.resize(ufunc_offset + ufuncs.size());
	for(uint i = 0; i < ufuncs.size(); ++i)
	{
		ParseFunction& f = *ufuncs[i];
		UserFunction& uf = module->ufuncs[i + ufunc_offset];
		uf.index = i + ufunc_offset;
		uf.name = GetName(&f);
		uf.pos = f.pos;
		uf.locals = f.locals;
		uf.result = f.result;
		for(auto& arg : f.arg_infos)
			uf.args.push_back(arg.vartype);
		uf.type = f.type;
		if(f.IsBuiltin())
			uf.pos = -1;
	}

	// convert types dtor from parse function to script function
	for(int i = tmp_type_offset, count = (int)module->tmp_types.size(); i < count; ++i)
	{
		Type* type = module->tmp_types[i];
		if(!type->dtor)
			continue;
		assert(type->dtor.type == AnyFunction::PARSE);
		type->dtor = &module->ufuncs[type->dtor.pf->index + ufunc_offset];
	}

	module->globals = main_block->GetMaxVars();
}

void Parser::Cleanup()
{
	DeleteElements(rsvs);
	StringPool.Free(tmp_strs);
	if(global_node)
	{
		global_node->Free();
		global_node = nullptr;
	}
}

void Parser::Reset()
{
	if(main_block)
	{
		main_block->Free();
		main_block = nullptr;
	}
	for(Type* type : module->tmp_types)
		t.RemoveKeyword(type->name.c_str(), type->index, G_VAR);
	DeleteElements(ufuncs);
}

void Parser::ParseCode()
{
	breakable_block = 0;
	if(!main_block)
	{
		main_block = Block::Get();
		main_block->parent = nullptr;
		main_block->var_offset = 0u;
	}
	current_block = main_block;
	current_function = nullptr;
	current_type = nullptr;
	global_node = nullptr;
	active_enum = nullptr;

	NodeRef node;
	node->pseudo_op = GROUP;
	node->result = V_VOID;
	node->source = nullptr;

	// first pass
	t.Next();
	AnalyzeCode();

	// second pass
	t.Next();
	while(!t.IsEof())
	{
		ParseNode* child = ParseLineOrBlock();
		if(child)
			node->push(child);
	}

	global_node = node.Pin();
}

// can return null
ParseNode* Parser::ParseLineOrBlock()
{
	if(t.IsSymbol('{'))
		return ParseBlock();
	else
	{
#ifdef _DEBUG
		uint current_line = t.GetLine();
		ParseNode* line = ParseLine();
		if(line && current_line != prev_line)
		{
			prev_line = current_line;
			ParseNode* group = ParseNode::Get();
			group->pseudo_op = GROUP;
			group->result = V_VOID;
			group->source = nullptr;
			ParseNode* line_mark = ParseNode::Get();
			line_mark->op = LINE;
			line_mark->value = current_line;
			line_mark->source = nullptr;
			line_mark->result = V_VOID;
			group->push(line_mark);
			group->push(line);
			return group;
		}
		else
			return line;
#else
		return ParseLine();
#endif
	}
}

// can return null
ParseNode* Parser::ParseBlock(ParseFunction* f)
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
	NodeRef group;
	while(true)
	{
		if(t.IsSymbol('}'))
			break;
		ParseNode* node = ParseLineOrBlock();
		if(node)
			group->push(node);
	}
	t.Next();

	if(!f)
	{
		// cleanup block if it's not function main block
		for(ParseVar* var : current_block->vars)
		{
			Type* var_type = GetType(var->vartype.GetType());
			if(var->referenced || var_type->dtor || var_type->IsRefCounted())
			{
				assert(var->referenced ^ (var_type->dtor || var_type->IsRefCounted()));
				if(!var->referenced)
					assert(var->subtype == ParseVar::LOCAL);
				ParseNode* rel = ParseNode::Get();
				rel->op = (var->referenced ? RELEASE_REF : RELEASE_OBJ);
				rel->value = (var->subtype == ParseVar::LOCAL ? var->index : -var->index - 1);
				rel->result = V_VOID;
				rel->source = nullptr;
				group->push(rel);
			}
		}
	}

	current_block = old_block;

	if(group->childs.empty())
		return nullptr;
	else
	{
		group->pseudo_op = GROUP;
		group->result = V_VOID;
		group->source = nullptr;
		return group.Pin();
	}
}

// can return null
ParseNode* Parser::ParseLine()
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

				NodeRef if_op;
				if_op->pseudo_op = IF;
				if_op->result = V_VOID;
				if_op->source = nullptr;
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

				return if_op.Pin();
			}
		case K_DO:
			{
				t.Next();
				++breakable_block;
				NodeRef block = ParseLineOrBlock();
				--breakable_block;
				t.AssertKeyword(K_WHILE, G_KEYWORD);
				t.Next();
				NodeRef cond = ParseCond();
				ParseNode* do_whil = ParseNode::Get();
				do_whil->pseudo_op = DO_WHILE;
				do_whil->result = V_VOID;
				do_whil->value = DO_WHILE_NORMAL;
				do_whil->source = nullptr;
				do_whil->push(block.Pin());
				do_whil->push(cond.Pin());
				return do_whil;
			}
		case K_WHILE:
			{
				t.Next();
				NodeRef cond = ParseCond();
				++breakable_block;
				NodeRef block = ParseLineOrBlock();
				--breakable_block;
				ParseNode* whil = ParseNode::Get();
				whil->pseudo_op = WHILE;
				whil->result = V_VOID;
				whil->source = nullptr;
				whil->push(cond.Pin());
				whil->push(block.Pin());
				return whil;
			}
		case K_FOR:
			{
				t.Next();
				t.AssertSymbol('(');
				t.Next();

				NodeRef for1(nullptr), for2(nullptr), for3(nullptr);
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
				{
					for1 = ParseExpr();
					t.AssertSymbol(';');
				}
				t.Next();
				if(t.IsSymbol(';'))
					for2 = nullptr;
				else
				{
					for2 = ParseExpr();
					t.AssertSymbol(';');
				}
				t.Next();
				if(t.IsSymbol(')'))
					for3 = nullptr;
				else
				{
					for3 = ParseExpr();
					t.AssertSymbol(')');
				}
				t.Next();

				if(for2 && !TryCast(for2.Get(), V_BOOL))
					t.Throw("Condition expression with '%s' type.", GetTypeName(for2));

				NodeRef fo = ParseNode::Get();
				fo->pseudo_op = FOR;
				fo->result = V_VOID;
				fo->source = nullptr;
				fo->push(for1.Pin());
				fo->push(for2.Pin());
				fo->push(for3.Pin());
				++breakable_block;
				fo->push(ParseLineOrBlock());
				--breakable_block;
				current_block = old_block;
				return fo.Pin();
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
				br->result = V_VOID;
				br->source = nullptr;
				return br;
			}
		case K_RETURN:
			return ParseReturn();
		case K_CLASS:
		case K_STRUCT:
			ParseClass(keyword == K_STRUCT);
			return nullptr;
		case K_SWITCH:
			return ParseSwitch();
		case K_IMPLICIT:
		case K_DELETE:
		case K_STATIC:
			return ParseFunc();
		case K_ENUM:
			ParseEnum(false);
			return nullptr;
		default:
			t.Unexpected();
		}
	}
	else if(t.IsKeywordGroup(G_VAR))
	{
		// is this function or var declaration or ctor
		NextType next_type = GetNextType(false);
		switch(next_type)
		{
		case NT_VAR_DECL:
			{
				// var
				ParseNode* node = ParseVarTypeDecl();
				t.Next();
				return node;
			}
		case NT_FUNC:
			return ParseFunc();
		case NT_CALL:
		case NT_INVALID:
			// fallback to ParseExpr
			break;
		default:
			assert(0);
			break;
		}
	}

	NodeRef node = ParseExpr();

	// ;
	t.AssertSymbol(';');
	t.Next();

	return node.Pin();
}

ParseNode* Parser::ParseReturn()
{
	NodeRef ret = ParseNode::Get();
	ret->pseudo_op = RETURN;
	ret->result = V_VOID;
	ret->source = nullptr;
	t.Next();
	if(!t.IsSymbol(';'))
	{
		ret->push(ParseExpr());
		t.AssertSymbol(';');
	}
	if(current_function)
	{
		VarType req_type;
		if(current_function->special == SF_CTOR)
			req_type = V_VOID;
		else
			req_type = current_function->result;

		bool ok = true;
		cstring type_name = nullptr;
		if(ret->childs.empty())
		{
			if(req_type.type != V_VOID)
			{
				ok = false;
				type_name = GetType(V_VOID)->name.c_str();
			}
		}
		else
		{
			ParseNode*& r = ret->childs[0];
			if(!TryCast(r, req_type))
			{
				ok = false;
				type_name = GetName(r->result);
			}
		}

		if(!ok)
			t.Throw("Invalid return type '%s', %s '%s' require '%s' type.", type_name, current_function->type == V_VOID ? "function" : "method",
				GetName(current_function), GetName(req_type));
	}
	else
	{
		if(!ret->childs.empty())
		{
			ParseNode* r = ret->childs[0];
			if(r->result.type != V_BOOL && r->result.type != V_CHAR && r->result.type != V_INT && r->result.type != V_FLOAT && r->result.type != V_STRING
				&& !GetType(r->result.type)->IsEnum())
				t.Throw("Invalid type '%s' for global return.", GetName(r->result));
		}
	}
	t.Next();
	return ret.Pin();
}

void Parser::ParseClass(bool is_struct)
{
	if(current_block != main_block)
		t.Throw("%s can't be declared inside block.", is_struct ? "Struct" : "Class");

	// id
	t.Next();
	Type* type = GetType(t.MustGetKeywordId(G_VAR));
	t.Next();
	type->size = 0;

	// {
	t.AssertSymbol('{');
	t.Next();

	uint pad = 0;
	current_type = type;

	// [class_item ...] }
	while(!t.IsSymbol('}'))
	{
		if(t.IsKeyword(K_ENUM))
			t.Throw("Enum can't be declared inside class.");

		// member, method or ctor
		NextType next = GetNextType(false);
		switch(next)
		{
		case NT_VAR_DECL:
			ParseMemberDeclClass(type, pad);
			break;
		case NT_FUNC:
			{
				Ptr<ParseFunction> f;
				f->flags = 0;
				current_function = f;

				ParseFuncInfo(f, type, false);

				AnyFunction af = FindEqualFunction(type, AnyFunction(f));
				assert(af.IsParse());

				// block
				ParseFunction* func = af.pf;
				if(!func->IsDeleted())
				{
					current_function = func;
					if(current_function->special == SF_CTOR)
					{
						for(Member* m : type->members)
							m->used = Member::No;
					}
					func->node = ParseBlock(func);
					if(current_function->special == SF_CTOR)
					{
						bool any = false;
						for(Member* m : type->members)
						{
							if(m->used != Member::Set)
							{
								any = true;
								break;
							}
						}
						if(any)
						{
							ParseNode* group = ParseNode::Get();
							for(Member* m : type->members)
							{
								if(m->used != Member::Set)
								{
									ParseNode* node = ParseNode::Get();
									SetParseNodeFromMember(node, m);
									group->push(node);
									group->push(SET_THIS_MEMBER, m->index);
									group->push(POP);
								}
							}
							group->result = V_VOID;
							group->pseudo_op = INTERNAL_GROUP;
							if(func->node)
								func->node->childs.insert(func->node->childs.begin(), group);
							else
								func->node = group;
						}
					}
				}
				current_function = nullptr;
			}
			break;
		case NT_INVALID:
		case NT_CALL:
			t.ThrowExpecting("member or method declaration");
			break;
		}
	}
	t.Next();

	current_type = nullptr;
}

ParseNode* Parser::ParseSwitch()
{
	NodeRef swi;

	// ( expr )
	t.Next();
	t.AssertSymbol('(');
	t.Next();
	swi->push(ParseExpr());
	int type = swi->childs[0]->result.type;
	if(type != V_BOOL && type != V_CHAR && type != V_INT && type != V_FLOAT && type != V_STRING && !GetType(type)->IsEnum())
		t.Throw("Invalid switch type '%s'.", GetName(swi->childs[0]->result));
	t.AssertSymbol(')');
	t.Next();

	// {
	t.AssertSymbol('{');
	t.Next();

	// cases
	bool have_def = false;
	while(!t.IsSymbol('}'))
	{
		do
		{
			ParseNode* cas = ParseCase(swi);
			swi->push(cas);
		} while(t.IsKeyword(K_CASE, G_KEYWORD) || t.IsKeyword(K_DEFAULT, G_KEYWORD));

		if(t.IsSymbol('}'))
			break;

		NodeRef group;
		++breakable_block;
		do
		{
			group->push(ParseLineOrBlock());
		} while(!t.IsKeyword(K_CASE, G_KEYWORD) && !t.IsKeyword(K_DEFAULT, G_KEYWORD) && !t.IsSymbol('}'));
		--breakable_block;

		for(auto it = swi->childs.rbegin(), end = swi->childs.rend(); it != end; ++it)
		{
			ParseNode* node = *it;
			if(node->linked)
				break;
			node->linked = group.Get();
		}

		group->pseudo_op = CASE_BLOCK;
		swi->push(group.Pin());
	}

	// }
	t.Next();

	swi->pseudo_op = SWITCH;
	swi->result = V_VOID;
	return swi.Pin();
}

ParseNode* Parser::ParseCase(ParseNode* swi)
{
	NodeRef cas;

	if(t.IsKeyword(K_CASE, G_KEYWORD))
	{
		t.Next();
		NodeRef val = ParseConstExpr();
		Type* type = GetType(val->result.type);
		if(val->result != V_BOOL && val->result != V_CHAR && val->result != V_INT && val->result != V_FLOAT && val->result != V_STRING
			&& !type->IsEnum())
			t.Throw("Invalid case type '%s'.", GetName(val->result));
		if(!TryConstCast(val.Get(), swi->childs[0]->result))
			t.Throw("Can't cast case value from '%s' to '%s'.", GetTypeName(val), GetTypeName(swi->childs[0]));
		assert(val->op != CAST);
		for(uint i = 1, count = swi->childs.size(); i < count; ++i)
		{
			ParseNode* chi = swi->childs[i];
			if(chi->pseudo_op == DEFAULT_CASE || chi->pseudo_op == CASE_BLOCK)
				continue;
			assert(chi->result == val->result);
			cstring dup = nullptr;
			switch(val->result.type)
			{
			case V_BOOL:
				if(val->bvalue == chi->bvalue)
					dup = (val->bvalue ? "true" : "false");
				break;
			case V_CHAR:
				if(val->cvalue == chi->cvalue)
					dup = EscapeChar(val->cvalue);
				break;
			case V_INT:
				if(val->value == chi->value)
					dup = Format("%d", val->value);
				break;
			case V_FLOAT:
				if(val->fvalue == chi->fvalue)
					dup = Format("%g", val->fvalue);
				break;
			case V_STRING:
				if(module->strs[val->value]->s == module->strs[chi->value]->s)
					dup = module->strs[val->value]->s.c_str();
				break;
			default:
				assert(type->IsEnum());
				if(val->value == chi->value)
					dup = Format("%d", val->value);
				break;
			}
			if(dup)
				t.Throw("Case with value '%s' is already defined.", dup);
		}
		switch(val->result.type)
		{
		case V_BOOL:
			cas->bvalue = val->bvalue;
			break;
		case V_CHAR:
			cas->cvalue = val->cvalue;
			break;
		case V_INT:
			cas->value = val->value;
			break;
		case V_FLOAT:
			cas->fvalue = val->fvalue;
			break;
		case V_STRING:
			cas->value = val->value;
			break;
		default:
			assert(type->IsEnum());
			cas->value = val->value;
			break;
		}
		cas->pseudo_op = CASE;
		cas->result = val->result;
	}
	else if(t.IsKeyword(K_DEFAULT, G_KEYWORD))
	{
		for(uint i = 1, count = swi->childs.size(); i < count; ++i)
		{
			ParseNode* chi = swi->childs[i];
			if(chi->pseudo_op == DEFAULT_CASE)
				t.Throw("Default case already defined.");
		}
		t.Next();
		cas->pseudo_op = DEFAULT_CASE;
	}
	else
	{
		int k1 = K_CASE,
			k2 = K_DEFAULT,
			gr = G_KEYWORD;
		t.StartUnexpected().Add(tokenizer::T_KEYWORD, &k1, &gr).Add(tokenizer::T_KEYWORD, &k2, &gr).Throw();
	}

	t.AssertSymbol(':');
	t.Next();

	cas->linked = nullptr;
	return cas.Pin();
}

void Parser::ParseEnum(bool forward)
{
	if(current_block != main_block)
		t.Throw("Enum can't be declared inside block.");
	t.Next();

	if(!forward)
	{
		t.SkipToSymbol('}');
		t.Next();
		return;
	}

	// name
	bool ok = true;
	Type* type = nullptr;
	if(t.IsKeywordGroup(G_VAR))
	{
		type = GetType(t.GetKeywordId(G_VAR));
		if(type->declared)
			ok = false;
	}
	else if(t.IsItem())
	{
		const string& name = t.MustGetItem();
		CheckFindItem(name, false);
		type = AnalyzeAddType(name);
	}
	else
		ok = false;
	if(!ok)
		t.ThrowExpecting("enum name");
	type->declared = true;
	Enum* enu = new Enum;
	enu->type = type;
	type->size = 4;
	type->enu = enu;
	type->SetGenericType();
	active_enum = enu;
	t.Next();

	// {
	t.AssertSymbol('{');
	t.Next();

	// enum values
	if(!t.IsSymbol('}'))
	{
		while(true)
		{
			// name
			const string& enum_value = t.MustGetItem();
			if(enu->Find(enum_value))
				t.Throw("Enumerator '%s.%s' already defined.", type->name.c_str(), enum_value.c_str());
			enu->values.push_back(std::pair<string, int>(enum_value, 0));
			t.Next();

			// [= value]
			auto& entry = enu->values.back();
			if(t.IsSymbol('='))
			{
				t.Next();
				NodeRef expr = ParseConstExpr();
				if(!TryConstCast(expr.Get(), VarType(V_INT)))
					t.Throw("Enumerator require 'int' value, have '%s'.", GetTypeName(expr));
				entry.second = expr->value;
			}
			else if(enu->values.size() >= 2u)
				entry.second = enu->values[enu->values.size() - 2].second + 1;

			// , or }
			if(t.IsSymbol('}'))
				break;
			t.AssertSymbol(',');
			t.Next();
		}
	}

	// }
	t.Next();
	active_enum = nullptr;

	// add compare operator
	ParseFunction* f = new ParseFunction;
	f->arg_infos.push_back(ArgInfo(VarType(type->index, 0), 0, false));
	f->arg_infos.push_back(ArgInfo(VarType(type->index, 0), 0, false));
	f->block = nullptr;
	f->flags = CommonFunction::F_BUILTIN;
	f->index = ufuncs.size();
	f->name = "$opAssign";
	f->node = nullptr;
	f->required_args = 2;
	f->result = V_BOOL;
	f->special = SF_NO;
	f->type = type->index;
#ifdef _DEBUG
	f->decl = GetName(f);
#endif
	type->ufuncs.push_back(f);
	ufuncs.push_back(f);
}

// func_decl
Function* Parser::ParseFuncDecl(cstring decl, Type* type, bool builtin)
{
	assert(decl);

	Function* f = new Function;
	f->flags = (builtin ? CommonFunction::F_BUILTIN : CommonFunction::F_CODE);

	try
	{
		t.FromString(decl);
		t.Next();

		ParseFuncInfo(f, type, true);

		t.AssertEof();
	}
	catch(Tokenizer::Exception& e)
	{
		Event(EventType::Error, e.ToString());
		delete f;
		f = nullptr;
	}

	return f;
}

Member* Parser::ParseMemberDecl(cstring decl)
{
	Member* m = new Member;

	try
	{
		t.FromString(decl);
		t.Next();

		m->vartype = GetVarTypeForMember();

		m->name = t.MustGetItem();
		t.Next();

		t.AssertEof();
	}
	catch(Tokenizer::Exception& e)
	{
		Event(EventType::Error, e.ToString());
		delete m;
		m = nullptr;
	}

	return m;
}

void Parser::ParseMemberDeclClass(Type* type, uint& pad)
{
	VarType vartype = GetVarTypeForMember();
	Type* var_type = GetType(vartype.type);
	uint var_size;
	if(vartype.type == V_STRING)
	{
		var_size = 4u; // stored as pointer to Str
		type->have_complex_member = true;
	}
	else
		var_size = var_type->size;
	assert(var_size != 0 && (var_size == 1 || var_size % 4 == 0));

	do
	{
		int index;
		Member* m = type->FindMember(t.MustGetItem(), index);
		assert(m && m->vartype == vartype);
		t.Next();

		if(pad == 0 || var_size == 1)
		{
			m->offset = type->size;
			type->size += var_size;
			pad = (pad + var_size) % 4;
		}
		else
		{
			type->size += 4 - pad;
			m->offset = type->size;
			type->size += var_size;
			pad = 0;
		}

		if(t.IsSymbol('='))
		{
			t.Next();
			t.SkipTo([](Tokenizer& t) { return t.IsSymbol(',') || t.IsSymbol(';'); });
		}
		else if(t.IsSymbol('('))
		{
			t.ForceMoveToClosingSymbol('(', ')');
			t.Next();
		}
		
		if(t.IsSymbol(';'))
			break;
		t.AssertSymbol(',');
		t.Next();
	} while(1);

	t.Next();
}

ParseNode* Parser::ParseFunc()
{
	// function
	if(current_block != main_block)
		t.Throw("Function can't be declared inside block.");
	Ptr<ParseFunction> f;
	f->flags = 0;
	current_function = f;
	ParseFuncInfo(f, nullptr, false);
	AnyFunction af = FindEqualFunction(f);
	assert(af.IsParse());

	// block
	ParseFunction* func = af.pf;
	if(!func->IsDeleted())
	{
		current_function = func;
		func->node = ParseBlock(func);
	}
	current_function = nullptr;

	// call
	if(t.IsSymbol('('))
	{
		NodeRef call = ParseExpr(func);
		t.AssertSymbol(';');
		return call.Pin();
	}
	else
		return nullptr;
}

void Parser::ParseFuncModifiers(bool have_type, int& flags)
{
	while(true)
	{
		if(t.IsKeywordGroup(G_KEYWORD))
		{
			int id = t.GetKeywordId(G_KEYWORD);
			bool ok = true;
			switch(id)
			{
			case K_IMPLICIT:
				if(!have_type)
					t.Throw("Implicit can only be used for methods.");
				if(IS_SET(flags, CommonFunction::F_IMPLICIT))
					t.Throw("Implicit already declared for this method.");
				flags |= CommonFunction::F_IMPLICIT;
				break;
			case K_DELETE:
				if(IS_SET(flags, CommonFunction::F_DELETE))
					t.Throw("Delete already declared for this %s.", have_type ? "method" : "function");
				flags |= CommonFunction::F_DELETE;
				break;
			case K_STATIC:
				if(!have_type)
					t.Throw("Static can only be used for methods.");
				if(IS_SET(flags, CommonFunction::F_STATIC))
					t.Throw("Static already declared for this method.");
				flags |= CommonFunction::F_STATIC;
				break;
			default:
				ok = false;
				break;
			}
			if(ok)
				t.Next();
			else
				break;
		}
		else
			break;
	}
}

void Parser::ParseFuncInfo(CommonFunction* f, Type* type, bool in_cpp)
{
	ParseFuncModifiers(type != nullptr, f->flags);

	bool oper = false, dtor = false;
	BASIC_SYMBOL symbol = BS_MAX;

	if(t.IsSymbol('~'))
	{
		dtor = true;
		t.Next();
	}

	f->result = GetVarType(false, in_cpp);
	f->type = (type ? type->index : V_VOID);

	if(t.IsSymbol('('))
	{
		// ctor/dtor
		if(type && type->index == f->result.type)
		{
			f->special = (dtor ? SF_DTOR : SF_CTOR);
			f->name = type->name;
		}
		else
			t.Throw("%s can only be declared inside class.", dtor ? "Destructor" : "Constructor");
	}
	else
	{
		if(dtor)
			t.Throw("Destructor tag mismatch.");

		if(in_cpp)
		{
			if(f->result.type == V_REF)
			{
				Type* result_type = GetType(f->result.subtype);
				if(result_type->IsRefClass())
				{
					f->result.type = f->result.subtype;
					f->result.subtype = 0;
				}
			}
			else
			{
				Type* result_type = GetType(f->result.type);
				if(result_type->IsRefClass())
					t.Throw("Class in code %s must be returned by reference/pointer.", type ? "method" : "function");
			}
		}

		f->special = SF_NO;
		if(t.IsKeyword(K_OPERATOR, G_KEYWORD))
		{
			oper = true;
			t.Next();
			if(t.IsItem())
			{
				const string& item = t.GetItem();
				if(item == "cast")
				{
					t.Next();
					f->special = SF_CAST;
					f->name = "$opCast";
				}
				else if(item == "addref")
				{
					t.Next();
					f->special = SF_ADDREF;
					f->name = "$opAddref";
				}
				else if(item == "release")
				{
					t.Next();
					f->special = SF_RELEASE;
					f->name = "$opRelease";
				}
				else
					t.Unexpected();
			}
			else
			{
				symbol = GetSymbol(true);
				if(symbol == BS_MAX)
					t.Unexpected();
				if(!CanOverload(symbol))
					t.Throw("Can't overload operator '%s'.", basic_symbols[symbol].GetOverloadText());
				t.Next();
			}
		}
		else
		{
			f->name = t.MustGetItem();
			t.Next();
		}
	}

	t.AssertSymbol('(');
	t.Next();

	ParseFunctionArgs(f, in_cpp);

	if(type && (f->special != SF_CTOR || !in_cpp) && !f->IsStatic())
	{
		// pass this for member functions, except for:
		//		static methods
		//		code constructors
		f->arg_infos.insert(f->arg_infos.begin(), ArgInfo(VarType((CoreVarType)type->index), 0, false));
		f->required_args++;
		if(in_cpp && !f->IsBuiltin())
			f->arg_infos.front().pass_by_ref = true;
	}

	if(f->IsImplicit())
	{
		if(f->special == SF_CTOR)
		{
			uint required = (in_cpp ? 1u : 2u);
			if(f->arg_infos.size() != required)
				t.Throw("Implicit constructor require single argument.");
		}
		else if(f->special != SF_CAST)
			t.Throw("Implicit can only be used for constructor and cast operators.");
	}

	if(f->IsStatic())
	{
		if(f->special == SF_CTOR)
			t.Throw("Static constructor not allowed.");
		else if(oper)
			t.Throw("Static operator not allowed.");
	}
	
	if(f->special == SF_CAST || f->special == SF_ADDREF || f->special == SF_RELEASE)
	{
		if(f->arg_infos.size() != 1u // first arg is this
			&& (f->special == SF_CAST || f->result.type == V_VOID)) // returns void or is cast
			t.Throw("Invalid cast operator definition '%s'.", GetName(f));
	}
	else if(f->special == SF_DTOR)
	{
		if(f->arg_infos.size() != 1u)
			t.Throw("Destructor can't have arguments.");
		if(type->IsRefCounted())
			t.Throw("Reference counted type can't have destructor.");
		if(in_cpp)
		{
			AnyFunction dtor(f, in_cpp ? AnyFunction::CODE : AnyFunction::PARSE);
			if(type->dtor)
			{
				if(type->dtor != dtor)
					t.Throw("Type '%s' have already declared destructor.", type->name.c_str());
			}
			else
				type->dtor = dtor;
		}
	}

	if(symbol != BS_MAX)
	{
		if(!FindMatchingOverload(*f, symbol))
			t.Throw("Invalid overload operator definition '%s'.", GetName(f, true, true, &symbol));
	}
}

void Parser::ParseFunctionArgs(CommonFunction* f, bool in_cpp)
{
	assert(f);
	f->required_args = 0;

	// args
	bool prev_arg_def = false;
	if(!t.IsSymbol(')'))
	{
		while(true)
		{
			VarType vartype = GetVarType(true, in_cpp);
			bool pass_by_ref = false;
			LocalString id = t.MustGetItem();
			CheckFindItem(id.get_ref(), false);
			if(!in_cpp)
			{
				ParseVar* arg = ParseVar::Get();
				arg->name = id;
				arg->vartype = vartype;
				arg->subtype = ParseVar::ARG;
				arg->index = current_function->args.size();
				arg->mod = false;
				arg->is_code_class = GetType(vartype.type)->IsCode();
				arg->referenced = false;
				current_function->args.push_back(arg);
			}
			else
			{
				if(vartype.type == V_REF && IS_SET(GetType(vartype.subtype)->flags, Type::Class | Type::PassByValue))
				{
					vartype.type = vartype.subtype;
					vartype.subtype = 0;
					pass_by_ref = true;
				}
			}
			t.Next();
			if(t.IsSymbol('='))
			{
				prev_arg_def = true;
				t.Next();
				NodeRef item = ParseConstExpr();
				if(!TryConstCast(item.Get(), vartype))
					t.Throw("Invalid default value of type '%s', required '%s'.", GetTypeName(item), GetName(vartype));
				switch(item->op)
				{
				case PUSH_BOOL:
					f->arg_infos.push_back(ArgInfo(item->bvalue));
					break;
				case PUSH_CHAR:
					f->arg_infos.push_back(ArgInfo(item->cvalue));
					break;
				case PUSH_INT:
					f->arg_infos.push_back(ArgInfo(item->value));
					break;
				case PUSH_FLOAT:
					f->arg_infos.push_back(ArgInfo(item->fvalue));
					break;
				case PUSH_STRING:
					f->arg_infos.push_back(ArgInfo(VarType(V_STRING), item->value, true));
					break;
				case PUSH_ENUM:
					f->arg_infos.push_back(ArgInfo(VarType(item->result.type, 0), item->value, true));
					break;
				default:
					assert(0);
					break;
				}
			}
			else
			{
				if(prev_arg_def)
					t.Throw("Missing default value for argument '%s'.", id.c_str());
				f->arg_infos.push_back(ArgInfo(vartype, 0, false));
				f->required_args++;
			}
			f->arg_infos.back().pass_by_ref = pass_by_ref;
			if(t.IsSymbol(')'))
				break;
			t.AssertSymbol(',');
			t.Next();
		}
	}
	t.Next();
}

ParseNode* Parser::ParseVarTypeDecl()
{
	// var_type
	VarType vartype = GetVarType(false, false);
	if(vartype.type == V_VOID)
		t.Throw("Can't declare void variable.");

	// var_decl(s)
	NodeVectorRef nodes;
	do
	{
		ParseNode* decl = ParseVarDecl(vartype);
		if(decl)
			nodes->push_back(decl);
		if(t.IsSymbol(';'))
			break;
		t.AssertSymbol(',');
		t.Next();
	} while(true);

	// node
	if(nodes->empty())
		return nullptr;
	else
	{
		ParseNode* node = ParseNode::Get();
		node->pseudo_op = GROUP;
		node->result = V_VOID;
		node->source = nullptr;
		node->childs = nodes.Pin();
		return node;
	}
}

ParseNode* Parser::ParseCond()
{
	t.AssertSymbol('(');
	t.Next();
	NodeRef cond = ParseExpr();
	t.AssertSymbol(')');
	if(!TryCast(cond.Get(), V_BOOL))
		t.Throw("Condition expression with '%s' type.", GetTypeName(cond));
	t.Next();
	return cond.Pin();
}

ParseNode* Parser::GetDefaultValueForVarDecl(VarType vartype)
{
	NodeRef expr;
	expr->source = nullptr;

	switch(vartype.type)
	{
	case V_BOOL:
		expr->result = V_BOOL;
		expr->pseudo_op = PUSH_BOOL;
		expr->bvalue = false;
		break;
	case V_CHAR:
		expr->result = V_CHAR;
		expr->op = PUSH_CHAR;
		expr->cvalue = 0;
		break;
	case V_INT:
		expr->result = V_INT;
		expr->op = PUSH_INT;
		expr->value = 0;
		break;
	case V_FLOAT:
		expr->result = V_FLOAT;
		expr->op = PUSH_FLOAT;
		expr->fvalue = 0.f;
		break;
	case V_STRING:
		expr->result = V_STRING;
		expr->op = PUSH_STRING;
		expr->value = GetEmptyString();
		break;
	case V_REF:
		t.Throw("Uninitialized reference variable.");
	default: // class
		{
			Type* rtype = GetType(vartype.type);
			if(rtype->IsClass())
			{
				if(IS_SET(rtype->flags, Type::DisallowCreate))
					t.Throw("Type '%s' cannot be created in script.", rtype->name.c_str());
				vector<AnyFunction> funcs;
				FindAllCtors(rtype, funcs);
				ApplyFunctionCall(expr, funcs, rtype, true);
			}
			else
			{
				assert(rtype->IsEnum());
				expr->result = vartype;
				expr->op = PUSH_ENUM;
				expr->value = 0;
			}
		}
		break;
	}

	return expr.Pin();
}

ParseNode* Parser::ParseVarCtor(VarType vartype)
{
	t.AssertSymbol('(');
	NodeRef node;
	node->source = nullptr;
	ParseArgs(node->childs);

	Type* type = GetType(vartype.type);
	if(IS_SET(type->flags, Type::DisallowCreate))
		t.Throw("Type '%s' cannot be created in script.", type->name.c_str());

	if(type->IsBuiltin() || vartype.type == V_REF)
	{
		if(node->childs.empty())
			return GetDefaultValueForVarDecl(vartype);
		else if(node->childs.size() == 1u)
		{
			if(!TryCast(node->childs[0], vartype))
				t.Throw("Can't assign type '%s' to type '%s'.", GetTypeName(node->childs[0]), GetName(vartype));
			ParseNode* result = node->childs[0];
			node->childs.clear();
			return result;
		}
		else
			t.Throw("Constructor for type '%s' require single argument.", type->name.c_str());
	}
	else
	{
		vector<AnyFunction> funcs;
		FindAllCtors(type, funcs);
		if(ApplyFunctionCall(node, funcs, type, true))
		{
			// builtin
			node->op = COPY;
			node->result = vartype;
		}
		return node.Pin();
	}
}

ParseNode* Parser::ParseVarDecl(VarType vartype)
{
	// var_name
	const string& name = t.MustGetItem();
	CheckFindItem(name, false);

	ParseVar* var = ParseVar::Get();
	var->name = name;
	var->vartype = vartype;
	var->index = current_block->var_offset;
	var->subtype = (current_function == nullptr ? ParseVar::GLOBAL : ParseVar::LOCAL);
	var->mod = false;
	var->is_code_class = GetType(vartype.type)->IsCode();
	var->referenced = false;
	current_block->vars.push_back(var);
	current_block->var_offset++;
	t.Next();

	int type; // 0-none, 1-assign, 2-ref assign, 3 - ctor syntax
	if(t.IsSymbol('='))
		type = 1;
	else if(t.IsSymbol('-') && t.PeekSymbol('>'))
		type = 2;
	else if(t.IsSymbol('('))
		type = 3;
	else
		type = 0;

	NodeRef expr(nullptr);
	bool require_copy = false, require_deref = false;

	if(type == 0)
		expr = GetDefaultValueForVarDecl(vartype);
	else if(type != 3)
	{
		if(type == 2 && vartype.type != V_REF)
			t.Unexpected();
		t.Next();

		expr = ParseExpr();

		Type* var_type = GetType(expr->result.GetType());
		if(vartype.type != V_REF && var_type->IsStruct() && (!IsCtor(expr) || var_type->index != vartype.type))
		{
			ParseNode* node = ParseNode::Get();
			node->source = nullptr;
			node->push(expr.Pin());
			expr = node;
			vector<AnyFunction> funcs;
			FindAllCtors(var_type, funcs);
			AnyFunction builtin = ApplyFunctionCall(node, funcs, var_type, true);
			if(builtin)
			{
				require_copy = true;
				ParseNode* old_expr = node->childs[0];
				node->childs.clear();
				expr = old_expr;
				if(expr->result.type == V_REF)
					require_deref = true;
			}
		}
		else
		{
			if(!TryCast(expr.Get(), vartype))
				t.Throw("Can't assign type '%s' to variable '%s'.", GetTypeName(expr), GetName(var));
		}
	}
	else
		expr = ParseVarCtor(vartype);

	ParseNode* node = ParseNode::Get();
	node->op = (var->subtype == ParseVar::GLOBAL ? SET_GLOBAL : SET_LOCAL);
	node->result = var->vartype;
	node->value = var->index;
	node->source = nullptr;
	node->push(expr.Pin());
	if(require_deref)
		node->push(DEREF);
	if(require_copy)
		node->push(COPY);
	return node;
}

ParseNode* Parser::ParseExpr(ParseFunction* func)
{
	vector<SymbolNode> stack, exit;
	vector<ParseNode*> stack2;

	try
	{
		ParseExprConvertToRPN(exit, stack, func);

		for(SymbolNode& sn : exit)
		{
			if(sn.is_symbol)
			{
				ParseExprApplySymbol(stack2, sn);
				for(ParseNode* node : stack2)
					assert(node);
			}
			else
			{
				assert(sn.node);
				stack2.push_back(sn.node);
				sn.node = nullptr;
			}
		}

		if(stack2.size() != 1u)
			t.Throw("Invalid operations.");

		return stack2.back();
	}
	catch(...)
	{
		for(SymbolNode& sn : stack)
		{
			if(sn.node)
				sn.node->Free();
		}

		for(SymbolNode& sn : exit)
		{
			if(sn.node)
				sn.node->Free();
		}

		ParseNode::Free(stack2);

		throw;
	}
}

void Parser::ParseExprConvertToRPN(vector<SymbolNode>& exit, vector<SymbolNode>& stack, ParseFunction* func)
{
	while(true)
	{
		BASIC_SYMBOL left = ParseExprPart(exit, stack, func);
		func = nullptr;
	next_symbol:
		if(GetNextSymbol(left))
		{
			if(left == BS_TERNARY)
			{
				PushSymbol(S_TERNARY, exit, stack);
				t.Next();

				NodeRef ter;
				ter->push(ParseExpr());
				t.AssertSymbol(':');
				t.Next();
				ter->push(ParseExpr());
				ParseNode*& leftn = ter->childs[0];
				ParseNode*& rightn = ter->childs[1];
				VarType common = CommonType(leftn->result, rightn->result);
				if(common.type == -1)
					t.Throw("Invalid common type for ternary operator with types '%s' and '%s'.", GetTypeName(leftn), GetTypeName(rightn));
				Cast(leftn, common);
				Cast(rightn, common);
				ter->pseudo_op = TERNARY_PART;
				ter->result = common;
				ter->source = nullptr;
				exit.push_back(ter.Pin());

				left = BS_MAX;
				goto next_symbol;
			}

			BasicSymbolInfo& bsi = basic_symbols[left];
			if(bsi.op_symbol == S_INVALID)
				t.Unexpected();
			PushSymbol(bsi.op_symbol, exit, stack);
			t.Next();

			if(bsi.op_symbol == S_MEMBER_ACCESS)
			{
				string* str = StringPool.Get();
				tmp_strs.push_back(str);
				*str = t.MustGetItem();
				t.Next();
				NodeRef node = ParseNode::Get();
				node->result = V_VOID;
				node->str = str;
				node->source = nullptr;
				if(t.IsSymbol('('))
				{
					ParseArgs(node->childs);
					node->pseudo_op = OBJ_FUNC;
				}
				else
					node->pseudo_op = OBJ_MEMBER;
				exit.push_back(node.Pin());
				left = BS_MAX;

				ParseExprPartPost(left, exit, stack);

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
}

BASIC_SYMBOL Parser::ParseExprPart(vector<SymbolNode>& exit, vector<SymbolNode>& stack, ParseFunction* func)
{
	BASIC_SYMBOL symbol = BS_MAX;

	if(func)
		exit.push_back(ParseItem(func));
	else
	{
		// [pre_op ...]
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
		{
			if(symbol == BS_CALL)
				symbol = BS_MAX;
			else
				t.Unexpected();
		}
		if(t.IsSymbol('('))
		{
			t.Next();
			exit.push_back(ParseExpr());
			t.AssertSymbol(')');
			t.Next();
		}
		else
			exit.push_back(ParseItem());
	}

	ParseExprPartPost(symbol, exit, stack);

	return symbol;
}

void Parser::ParseExprPartPost(BASIC_SYMBOL& symbol, vector<SymbolNode>& exit, vector<SymbolNode>& stack)
{
	while(GetNextSymbol(symbol))
	{
		BasicSymbolInfo& bsi = basic_symbols[symbol];
		if(bsi.post_symbol == S_INVALID)
			return;

		if(bsi.post_symbol == S_SUBSCRIPT)
		{
			NodeRef sub;
			ParseArgs(sub->childs, '[', ']');
			sub->pseudo_op = SUBSCRIPT;
			sub->source = nullptr;
			PushSymbol(bsi.post_symbol, exit, stack, sub.Pin());
		}
		else if(bsi.post_symbol == S_CALL)
		{
			NodeRef sub;
			ParseArgs(sub->childs);
			sub->pseudo_op = CALL_FUNCTOR;
			sub->source = nullptr;
			PushSymbol(bsi.post_symbol, exit, stack, sub.Pin());
		}
		else
		{
			PushSymbol(bsi.post_symbol, exit, stack);
			t.Next();
		}

		symbol = BS_MAX;
	}
}

void Parser::ParseExprApplySymbol(vector<ParseNode*>& stack, SymbolNode& sn)
{
	SymbolInfo& si = symbols[sn.symbol];
	if(si.args > (int)stack.size())
		t.Throw("Missing arguments on stack for operator '%s'.", si.name);

	if(si.args == 1)
	{
		NodeRef node = stack.back();
		stack.pop_back();
		if(si.type == ST_NONE)
		{
			// unrary operator
			OpResult op_result = CanOp(sn.symbol, sn.symbol, node, nullptr);
			if(op_result.result == OpResult::NO)
				t.Throw("Invalid type '%s' for operation '%s'.", GetTypeName(node), si.name);
			if(op_result.result == OpResult::OVERLOAD)
			{
				node.Pin();
				stack.push_back(op_result.over_result);
			}
			else
			{
				assert(op_result.result == OpResult::YES);
				Cast(node.Get(), op_result.cast_var);
				if(!TryConstExpr1(node, si.symbol) && si.op != NOP)
				{
					ParseNode* op = ParseNode::Get();
					op->op = (Op)si.op;
					op->result = op_result.result_var;
					op->push(node.Pin());
					op->source = nullptr;
					node = op;
				}
				stack.push_back(node.Pin());
			}
		}
		else if(si.type == ST_SUBSCRIPT)
		{
			// subscript operator
			Type* ltype = GetType(node->result.GetType());
			vector<AnyFunction> funcs;
			FindAllFunctionOverloads(ltype, si.op_code, funcs);
			if(funcs.empty())
				t.Throw("Type '%s' don't have subscript operator.", GetTypeName(node));

			NodeRef op;
			op->source = node->source;
			op->push(node.Pin());
			op->push(sn.node->childs);
			sn.node->childs.clear();
			sn.node->Free();
			sn.node = nullptr;

			AnyFunction builtin = ApplyFunctionCall(op.Get(), funcs, ltype, false, true);
			if(builtin)
			{
				Cast(op->childs[0], V_STRING); // cast for dereference
				op->op = PUSH_INDEX;
				op->result = VarType(V_REF, V_CHAR);
			}
			
			stack.push_back(op.Pin());
		}
		else if(si.type == ST_CALL)
		{
			// call operator
			Type* type = GetType(node->result.GetType());
			vector<AnyFunction> funcs;
			FindAllFunctionOverloads(type, si.op_code, funcs);
			if(funcs.empty())
				t.Throw("Type '%s' don't have call operator.", GetTypeName(node));

			NodeRef f;
			f->source = node->source;
			f->push(node.Pin());
			f->push(sn.node->childs);
			sn.node->childs.clear();
			sn.node->Free();
			sn.node = nullptr;

			ApplyFunctionCall(f, funcs, type, false);

			stack.push_back(f.Pin());
		}
		else
		{
			// inc dec
			assert(si.type == ST_INC_DEC);
			OpResult op_result = CanOp(si.symbol, si.symbol, node, nullptr);
			if(op_result.result == OpResult::OVERLOAD)
			{
				node.Pin();
				stack.push_back(op_result.over_result);
			}
			else
			{
				assert(op_result.result == OpResult::FALLBACK);
				int type = node->result.GetType();
				if(type != V_CHAR && type != V_INT && type != V_FLOAT)
					t.Throw("Invalid type '%s' for operation '%s'.", GetTypeName(node), si.name);

				bool pre = (si.symbol == S_PRE_INC || si.symbol == S_PRE_DEC);
				bool inc = (si.symbol == S_PRE_INC || si.symbol == S_POST_INC);
				Op oper = (inc ? INC : DEC);

				NodeRef op;
				op->pseudo_op = INTERNAL_GROUP;
				op->result = VarType(node->result.GetType(), 0);
				op->source = nullptr;
				if(node->source)
					node->source->mod = true;

				if(node->result.type != V_REF)
				{
					Op set_op = PushToSet(node);
					if(set_op == NOP)
						t.Throw("Operation '%s' require variable.", si.name);

					ParseNode* pnode = node.Pin();

					if(set_op == SET_MEMBER)
					{
						// inc dec member var
						op->push(pnode->childs);
						pnode->childs.clear();
						op->push(PUSH);
						op->push(pnode);
						if(pre)
						{
							op->push(oper);
							op->push(SET_MEMBER, pnode->value);
						}
						else
						{
							op->push(SET_TMP);
							op->push(oper);
							op->push(SET_MEMBER, pnode->value);
							op->push(POP);
							op->push(PUSH_TMP);
						}
					}
					else if(pre)
					{
						op->push(pnode);
						op->push(oper);
						op->push(set_op, pnode->value);
					}
					else
					{
						op->push(pnode);
						op->push(PUSH);
						op->push(oper);
						op->push(set_op, pnode->value);
						op->push(POP);
					}
				}
				else
				{
					ParseNode* pnode = node.Pin();
					if(pre)
					{
						op->push(pnode);
						op->push(PUSH);
						op->push(DEREF);
						op->push(oper);
						op->push(SET_ADR);
					}
					else
					{
						op->push(pnode);
						op->push(PUSH);
						op->push(DEREF);
						op->push(PUSH);
						op->push(SWAP, 1);
						op->push(oper);
						op->push(SET_ADR);
						op->push(POP);
					}
				}

				stack.push_back(op.Pin());
			}
		}
	}
	else
	{
		// two argument operation
		assert(si.args == 2);
		NodeRef right = stack.back();
		stack.pop_back();
		NodeRef left = stack.back();
		stack.pop_back();

		if(si.symbol == S_MEMBER_ACCESS)
		{
			// member access
			Type* type = GetType(left->result.GetType());
			bool deref = (left->result.type == V_REF);

			if(right->pseudo_op == OBJ_FUNC)
			{
				vector<AnyFunction> funcs;
				FindAllFunctionOverloads(type, *right->str, funcs);
				if(type->index == V_TYPE)
					FindAllStaticFunctionOverloads(GetType(left->value), *right->str, funcs);
				if(funcs.empty())
				{
					cstring err;
					if(type->index == V_TYPE)
						err = Format("Missing static method '%s' for type '%s'.", right->str->c_str(), GetType(left->value)->name.c_str());
					else
						err = Format("Missing method '%s' for type '%s'.", right->str->c_str(), type->name.c_str());
					t.Throw(err);
				}
				FreeTmpStr(right->str);

				NodeRef node = ParseNode::Get();
				node->source = left->source;
				if(!deref)
					node->push(left.Pin());
				else
				{
					ParseNode* d = ParseNode::Get();
					d->push(left.Pin());
					d->result = VarType(type->index, 0);
					d->op = DEREF;
					node->push(d);
				}
				for(ParseNode* n : right->childs)
					node->push(n);
				right->childs.clear();

				ApplyFunctionCall(node, funcs, type, false, true);

				right.Pin()->Free();
				stack.push_back(node.Pin());
			}
			else
			{
				assert(right->pseudo_op == OBJ_MEMBER);
				Type* real_type = type;
				if(type->index == V_TYPE)
					real_type = GetType(left->value);
				if(real_type->IsEnum())
				{
					auto e = real_type->enu->Find(*right->str);
					if(!e)
						t.Throw("Invalid enumerator '%s.%s'.", real_type->name.c_str(), right->str->c_str());
					FreeTmpStr(right->str);
					ParseNode* enu = ParseNode::Get();
					enu->op = PUSH_ENUM;
					enu->result = VarType(real_type->index, 0);
					enu->source = nullptr;
					enu->value = e->second;
					right.Pin()->Free();
					stack.push_back(enu);
				}
				else
				{
					int m_index;
					Member* m = type->FindMember(*right->str, m_index);
					if(!m)
						t.Throw("Missing member '%s' for type '%s'.", right->str->c_str(), type->name.c_str());
					FreeTmpStr(right->str);
					ParseNode* node = ParseNode::Get();
					node->op = PUSH_MEMBER;
					node->result = m->vartype;
					node->value = m_index;
					node->source = left->source;
					if(!deref)
						node->push(left.Pin());
					else
					{
						ParseNode* d = ParseNode::Get();
						d->push(left.Pin());
						d->result = VarType(type->index, 0);
						d->op = DEREF;
						node->push(d);
					}
					right.Pin()->Free();
					stack.push_back(node);
				}
			}
		}
		else if(si.symbol == S_TERNARY)
		{
			assert(right->pseudo_op == TERNARY_PART);
			if(!TryCast(left.Get(), V_BOOL))
				t.Throw("Ternary condition expression with '%s' type.", GetTypeName(left));
			NodeRef ter;
			ter->push(left.Pin());
			ter->push(right->childs[0]);
			ter->push(right->childs[1]);
			right->childs.clear();
			ter->pseudo_op = TERNARY;
			ter->result = ter->childs[1]->result;
			ter->source = nullptr;
			stack.push_back(ter.Pin());
		}
		else if(si.symbol == S_SET_REF)
		{
			// assign reference
			if(!CanTakeRef(left, false) || left->result.type != V_REF)
				t.Throw("Can't assign reference, left value must be reference variable.");
			if(!CanTakeRef(right))
				t.Throw("Can't assign reference, right value must be variable.");
			NodeRef set;
			set->source = nullptr;
			set->op = PushToSet(left);
			assert(set->op != NOP);
			set->value = left->value;
			set->result = left->result;

			// assign
			if(!TryCast(right.Get(), left->result))
				t.Throw("Can't reference assign '%s' to type '%s'.", GetTypeName(right), GetTypeName(set));
			if(left->op == PUSH_MEMBER || left->op == PUSH_INDEX)
			{
				set->push(left->childs);
				left->childs.clear();
			}
			set->push(right.Pin());
			left.Pin()->Free();
			stack.push_back(set.Pin());
		}
		else if(si.symbol == S_SET_LONG_REF)
		{
			if(left->result.type != V_REF || !GetType(left->result.subtype)->IsRefClass())
				t.Throw("Can't long assign reference, left value must be reference to class.");
			if(!CanTakeRef(right))
				t.Throw("Can't long assign reference, right value must be variable.");

			NodeRef set;
			set->source = nullptr;
			set->op = SET_ADR;
			set->result = VarType(left->result.subtype, 0);
			if(!TryCast(right.Get(), set->result))
				t.Throw("Can't long reference assign '%s' to type '%s'.", GetTypeName(right), GetTypeName(left));
			set->push(left.Pin());
			set->push(right.Pin());
			stack.push_back(set.Pin());
		}
		else if(si.type == ST_ASSIGN)
			stack.push_back(ParseAssign(si, left, right));
		else
		{
			// normal operation
			OpResult op_result = CanOp(si.symbol, si.symbol, left, right);
			switch(op_result.result)
			{
			case OpResult::FALLBACK:
				assert(0);
			case OpResult::NO:
				t.Throw("Invalid types '%s' and '%s' for operation '%s'.", GetTypeName(left), GetTypeName(right), si.name);
			case OpResult::YES:
				{
					ParseNode* pleft = left.Pin();
					ParseNode* pright = right.Pin();

					Cast(pleft, op_result.cast_var);
					Cast(pright, op_result.cast_var);

					ParseNode* op = ParseNode::Get();
					op->result = op_result.result_var;
					op->source = nullptr;

					if(!TryConstExpr(pleft, pright, op, si.symbol))
					{
						op->op = (Op)si.op;
						op->push(pleft);
						op->push(pright);
					}

					stack.push_back(op);
				}
				break;
			case OpResult::OVERLOAD:
				left.Pin();
				right.Pin();
				stack.push_back(op_result.over_result);
				break;
			case OpResult::CAST:
				Cast(left.Get(), VarType(right->value, 0), CF_EXPLICIT, &op_result.cast_result);
				stack.push_back(left.Pin());
				break;
			}
		}
	}
}

ParseNode* Parser::ParseAssign(SymbolInfo& si, NodeRef& left, NodeRef& right)
{
	if(!CanTakeRef(left) && !GetType(left->result.type)->IsAssignable())
		t.Throw("Can't assign, left must be assignable.");
	if(left->source)
		left->source->mod = true;

	NodeRef set;
	set->source = nullptr;

	if(si.op == NOP)
	{
		// assign to variable
		Type* left_type = GetType(left->result.GetType());

		// find all assign operators
		vector<AnyFunction> funcs;
		FindAllFunctionOverloads(left_type, si.op_code, funcs);
		if(funcs.empty())
			t.Throw("Invalid types '%s' and '%s' for operation '%s'.", GetTypeName(left), GetTypeName(right), si.name);

		// try to match correct call
		set->owned = false;
		set->push(left);
		set->push(right);
		AnyFunction builtin = ApplyFunctionCall(set, funcs, left_type, false);
		if(builtin)
		{
			// use builtin op instead of function call
			set->childs.clear();
			set->owned = true;
			if(left->result.type != V_REF)
			{
				set->op = PushToSet(left);
				if(set->op == NOP && left_type->IsPassByValue())
				{
					// works like struct, it is l-value
					set->pseudo_op = GROUP;
					set->result = left->result;
					if(!TryCast(right.Get(), left->result))
						t.Throw("Can't assign '%s' to type '%s'.", GetTypeName(right), GetTypeName(set));
					set->childs.clear();
					set->push(left.Pin());
					set->push(POP);
					set->push(right.Pin());
				}
				else
				{
					assert(set->op != NOP);
					set->value = left->value;
					set->result = left->result;
					if(!TryCast(right.Get(), left->result))
						t.Throw("Can't assign '%s' to type '%s'.", GetTypeName(right), GetTypeName(set));
					set->childs.clear();
					if(left->op == PUSH_MEMBER || left->op == PUSH_INDEX)
					{
						set->push(left->childs);
						left->childs.clear();
					}
					set->push(right.Pin());
					left.Pin()->Free();
				}
			}
			else
			{
				set->result = VarType(left->result.subtype, 0);
				if(!TryCast(right.Get(), VarType(left->result.subtype, 0)))
					t.Throw("Can't assign '%s' to type '%s'.", GetTypeName(right), GetTypeName(left));
				set->op = SET_ADR;
				set->push(left.Pin());
				set->push(right.Pin());
			}
		}
		else
		{
			set->owned = true;
			left.Pin();
			right.Pin();
		}
	}
	else
	{
		// compound assignment
		OpResult op_result = CanOp((SYMBOL)si.op, si.symbol, left, right);
		if(op_result.result == OpResult::NO)
			t.Throw("Invalid types '%s' and '%s' for operation '%s'.", GetTypeName(left), GetTypeName(right), si.name);

		if(op_result.result == OpResult::OVERLOAD)
		{
			left.Pin();
			right.Pin();
			set = op_result.over_result;
		}
		else if(left->result.type != V_REF)
		{
			set->op = PushToSet(left);
			if(set->op == NOP && GetType(left->result.type)->IsPassByValue())
			{
				// works like struct, it is l-value
				set->op = (Op)symbols[si.op].op;
				set->result = op_result.result_var;
				set->source = nullptr;
				set->owned = true;
				set->push(left.Pin());
				set->push(right.Pin());
			}
			else
			{
				assert(set->op != NOP);
				set->value = left->value;
				set->result = left->result;

				NodeRef op;
				op->op = (Op)symbols[si.op].op;
				op->result = op_result.result_var;
				op->source = nullptr;

				if(left->op == PUSH_MEMBER)
				{
					set->push(left->childs);
					set->push(PUSH);
					left->childs.clear();
					Cast(left.Get(), op_result.cast_var);
					Cast(right.Get(), op_result.cast_var);
					set->push(left.Pin());
					set->push(right.Pin());
					ForceCast(op.Get(), set->result, si.name);
					set->push(op.Pin());
				}
				else if(left->op == PUSH_INDEX)
				{
					assert(left->childs.size() == 2u); // push arr, push index
					ParseNode* push = ParseNode::Get();
					push->op = PUSH;
					left->childs.insert(left->childs.begin() + 1, push);
					left->push(PUSH);
					left->push(SWAP, 1);
					Cast(left.Get(), op_result.cast_var);
					Cast(right.Get(), op_result.cast_var);
					op->push(left.Pin());
					op->push(right.Pin());
					ForceCast(op.Get(), set->result, si.name);
					set->push(op.Pin());
				}
				else
				{
					Cast(left.Get(), op_result.cast_var);
					Cast(right.Get(), op_result.cast_var);

					op->push(left.Pin());
					op->push(right.Pin());

					ForceCast(op.Get(), set->result, si.name);
					set->push(op.Pin());
				}
			}
		}
		else
		{
			set->result = VarType(left->result.subtype, 0);

			// compound assign
			OpResult op_result = CanOp((SYMBOL)si.op, si.symbol, left, right);
			if(op_result.result == OpResult::NO)
				t.Throw("Invalid types '%s' and '%s' for operation '%s'.", GetTypeName(left), GetTypeName(right), si.name);

			assert(op_result.result == OpResult::YES); // compound assign on reference ???

			NodeRef real_left = left.Get()->copy();
			Cast(left.Get(), op_result.cast_var);
			Cast(right.Get(), op_result.cast_var);

			NodeRef op;
			op->op = (Op)symbols[si.op].op;
			op->result = op_result.result_var;
			op->source = nullptr;
			op->push(left.Pin());
			op->push(right.Pin());

			ForceCast(op.Get(), set->result, si.name);
			set->push(real_left.Pin());
			set->push(op.Pin());
			set->op = SET_ADR;
		}
	}

	return set.Pin();
}

ParseNode* Parser::ParseConstExpr()
{
	NodeRef expr = ParseExpr();
	bool is_const = false;
	switch(expr->op)
	{
	case PUSH_BOOL:
	case PUSH_CHAR:
	case PUSH_INT:
	case PUSH_FLOAT:
	case PUSH_STRING:
	case PUSH_ENUM:
		is_const = true;
		break;
	}
	if(!is_const)
		t.Throw("Const expression required.");
	return expr.Pin();
}

void Parser::ParseArgs(vector<ParseNode*>& nodes, char open, char close)
{
	// (
	t.AssertSymbol(open);
	t.Next();

	// arguments
	if(!t.IsSymbol(close))
	{
		while(true)
		{
			nodes.push_back(ParseExpr());
			if(t.IsSymbol(close))
				break;
			t.AssertSymbol(',');
			t.Next();
		}
	}
	t.Next();
}

ParseNode* Parser::ParseItem(ParseFunction* func)
{
	if(t.IsKeywordGroup(G_VAR))
	{
		VarType vartype(t.GetKeywordId(G_VAR), 0);
		t.Next();

		if(t.IsSymbol('('))
		{
			// ctor
			return ParseVarCtor(vartype);
		}
		else
		{
			// type
			ParseNode* type = ParseNode::Get();
			type->pseudo_op = PUSH_TYPE;
			type->result = V_TYPE;
			type->value = vartype.type;
			type->source = nullptr;
			return type;
		}
	}
	else if(t.IsItem() || func)
	{
		const string& id = t.GetTokenString(); // get item or ( if func is not null
		Found found;
		FOUND found_type;
		if(func)
			found_type = F_USER_FUNC;
		else
			found_type = FindItem(id, found);
		switch(found_type)
		{
		case F_VAR:
			{
				ParseVar* var = found.var;
				ParseNode* node = ParseNode::Get();
				node->result = var->vartype;
				node->value = var->index;
				node->source = var;
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
					if(current_type && !current_function->IsStatic())
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
				if(!func)
				{
					FindAllFunctionOverloads(id, funcs);
					t.Next();
				}
				else
					funcs.push_back(func);

				NodeRef node;
				node->source = nullptr;

				ParseArgs(node->childs);
				ApplyFunctionCall(node, funcs, nullptr, false);

				return node.Pin();
			}
		case F_MEMBER:
			{
				ParseNode* node = ParseNode::Get();
				node->op = PUSH_THIS_MEMBER;
				node->result = found.member->vartype;
				node->value = found.member_index;
				node->source = nullptr;
				if(found.member->used == Member::No)
					found.member->used = Member::Used;
				else
					found.member->used = Member::UsedBeforeSet;
				t.Next();
				return node;
			}
		default:
			assert(0);
			t.Unexpected();
			break;
		case F_NONE:
			if(active_enum)
			{
				const string& name = t.MustGetItem();
				auto e = active_enum->Find(name);
				if(e)
				{
					ParseNode* enu = ParseNode::Get();
					enu->op = PUSH_ENUM;
					enu->value = e->second;
					enu->source = nullptr;
					enu->result = VarType(active_enum->type->index, 0);
					t.Next();
					return enu;
				}
			}
			t.Unexpected();
			break;
		}
	}
	else
		return ParseConstItem();
}

ParseNode* Parser::ParseConstItem()
{
	if(t.IsInt())
	{
		// int
		int val = t.GetInt();
		ParseNode* node = ParseNode::Get();
		node->op = PUSH_INT;
		node->result = V_INT;
		node->value = val;
		node->source = nullptr;
		t.Next();
		return node;
	}
	else if(t.IsFloat())
	{
		// float
		float val = t.GetFloat();
		ParseNode* node = ParseNode::Get();
		node->op = PUSH_FLOAT;
		node->result = V_FLOAT;
		node->fvalue = val;
		node->source = nullptr;
		t.Next();
		return node;
	}
	else if(t.IsChar())
	{
		// char
		char c = t.GetChar();
		ParseNode* node = ParseNode::Get();
		node->op = PUSH_CHAR;
		node->result = V_CHAR;
		node->cvalue = c;
		node->source = nullptr;
		t.Next();
		return node;
	}
	else if(t.IsString())
	{
		// string
		int index = module->strs.size();
		Str* str = Str::Get();
		str->s = t.GetString();
		str->refs = 1;
		module->strs.push_back(str);
		ParseNode* node = ParseNode::Get();
		node->op = PUSH_STRING;
		node->value = index;
		node->result = V_STRING;
		node->source = nullptr;
		t.Next();
		return node;
	}
	else if(t.IsKeywordGroup(G_CONST))
	{
		CONST c = (CONST)t.GetKeywordId(G_CONST);
		t.Next();
		ParseNode* node = ParseNode::Get();
		switch(c)
		{
		case C_TRUE:
			node->pseudo_op = PUSH_BOOL;
			node->bvalue = true;
			node->result = V_BOOL;
			break;
		case C_FALSE:
			node->pseudo_op = PUSH_BOOL;
			node->bvalue = false;
			node->result = V_BOOL;
			break;
		case C_THIS:
			if(!current_type)
				t.Throw("This can be used only inside class.");
			node->op = PUSH_THIS;
			node->result = VarType(current_type->index, 0);
			break;
		default:
			assert(0);
			break;
		}
		node->source = nullptr;
		return node;
	}
	else
		t.ThrowExpecting("const item");
}

void Parser::CheckFindItem(const string& id, bool is_func)
{
	Found found;
	FOUND found_type = FindItem(id, found);
	if(found_type != F_NONE && !(is_func && (found_type == F_FUNC || found_type == F_USER_FUNC)))
		t.Throw("Name '%s' already used as %s.", id.c_str(), found.ToString(found_type));
}

ParseVar* Parser::GetVar(ParseNode* node)
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

VarType Parser::GetVarType(bool is_arg, bool in_cpp)
{
	VarType vartype(nullptr);
	bool ok = false;
	if(t.IsKeywordGroup(G_VAR))
	{
		vartype.type = t.GetKeywordId(G_VAR);
		ok = true;
	}
	if(!ok)
		t.Unexpected("Expecting var type.");
	t.Next();

	if(t.IsSymbol('&'))
	{
		t.Next();
		vartype.subtype = vartype.type;
		vartype.type = V_REF;
	}
	else if(is_arg && in_cpp)
	{
		Type* type = GetType(vartype.type);
		if(type->IsRef())
			t.Throw("Reference type '%s' must be passed by reference/pointer.", type->name.c_str());
	}

	return vartype;
}

VarType Parser::GetVarTypeForMember()
{
	VarType vartype = GetVarType(false);
	if(vartype.type == V_VOID)
		t.Throw("Member of 'void' type not allowed.");
	else
	{
		cstring name = nullptr;
		if(vartype.type == V_REF)
			name = "reference";
		else
		{
			Type* type = GetType(vartype.type);
			if(type->IsClass())
			{
				if(type->IsRef())
					name = "class";
				else
					name = "struct";
			}
		}
		if(name)
			t.Throw("Member of '%s' type not allowed yet.", name);
	}
	return vartype;
}

void Parser::PushSymbol(SYMBOL symbol, vector<SymbolNode>& exit, vector<SymbolNode>& stack, ParseNode* node)
{
	while(!stack.empty())
	{
		SymbolNode symbol2 = stack.back();
		SymbolInfo& s1 = symbols[symbol];
		SymbolInfo& s2 = symbols[symbol2.symbol];

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

	stack.push_back(SymbolNode(symbol, node));
}

bool Parser::GetNextSymbol(BASIC_SYMBOL& symbol)
{
	if(symbol != BS_MAX)
		return true;
	symbol = GetSymbol();
	return (symbol != BS_MAX);
}

BASIC_SYMBOL Parser::GetSymbol(bool full_over)
{
	if(t.IsKeywordGroup(G_KEYWORD))
	{
		KEYWORD k = (KEYWORD)t.GetKeywordId(G_KEYWORD);
		if(k == K_IS)
			return BS_IS;
		if(k == K_AS)
			return BS_AS;
	}
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
		{
			if(t.PeekSymbol('>'))
				return BS_SET_LONG_REF;
			else
				return BS_DEC;
		}
		else if(t.PeekSymbol('>'))
			return BS_SET_REF;
		else
			return BS_MINUS;
	case '*':
		if(t.PeekSymbol('='))
			return BS_ASSIGN_MUL;
		else if(t.PeekSymbol('/'))
			t.Throw("Unclosed multiline comment.");
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
	case '[':
		if(!full_over || t.PeekSymbol(']'))
			return BS_SUBSCRIPT;
		else
			return BS_MAX;
	case '~':
		return BS_BIT_NOT;
	case '(':
		if(!full_over || t.PeekSymbol(')'))
			return BS_CALL;
		else
			return BS_MAX;
	case '?':
		return BS_TERNARY;
	default:
		return BS_MAX;
	}
}

OpResult Parser::CanOp(SYMBOL symbol, SYMBOL real_symbol, ParseNode* lnode, ParseNode* rnode)
{
	OpResult op_result;
	VarType leftvar = lnode->result;
	VarType rightvar = (rnode ? rnode->result : V_VOID);
	int& cast = op_result.cast_var.type;
	int left = leftvar.GetType();
	int right = rightvar.GetType();

	// left is void, right is void for two arg op, left is type
	if(left == V_VOID || left == V_TYPE || (right == V_VOID && symbols[symbol].args != 1))
		return op_result;

	if(right == V_TYPE)
	{
		// when right is type, only as can be used
		if(symbol != S_AS)
			return op_result;
		VarType vartype(rnode->value, 0);
		op_result.cast_result = MayCast(lnode, vartype, false);
		if(op_result.cast_result.CantCast())
			t.Throw("Can't cast from '%s' to '%s'.", GetTypeName(lnode), GetName(vartype));
		op_result.result = OpResult::CAST;
		return op_result;
	}

	// check for overloads
	Type* ltype = GetType(left);
	Type* rtype = GetType(right);
	if((ltype->IsClass() || rtype->IsClass()) && symbol != S_IS)
	{
		// left type must be class
		if(!ltype->IsClass())
			return op_result;

		// not overloadable symbol ?
		SymbolInfo& si = symbols[real_symbol];
		if(!si.op_code)
			return op_result;

		// find all overloads
		vector<AnyFunction> funcs;
		FindAllFunctionOverloads(ltype, si.op_code, funcs);
		if(funcs.empty())
			return op_result;

		NodeRef node;
		node->push(lnode);
		node->source = nullptr;
		if(right)
			node->push(rnode);
		try
		{
			AnyFunction builtin = ApplyFunctionCall(node, funcs, ltype, false);
			if(builtin)
			{
				node->result = builtin.cf->result;
				node->op = (Op)si.op;
				Cast(node->childs[1], builtin.cf->arg_infos[1].vartype);
			}
			op_result.over_result = node.Pin();
			op_result.result = OpResult::OVERLOAD;
			return op_result;
		}
		catch(...)
		{
			node->childs.clear();
			throw;
		}
	}

	VarType vartype;
	switch(symbol)
	{
	case S_ADD:
		if(left == V_STRING || right == V_STRING)
			vartype = V_STRING;
		else if(left == V_FLOAT || right == V_FLOAT)
			vartype = V_FLOAT;
		else // int or char or bool or enum
			vartype = V_INT;
		op_result.cast_var = vartype;
		op_result.result_var = vartype;
		op_result.result = OpResult::YES;
		break;
	case S_SUB:
	case S_MUL:
	case S_DIV:
	case S_MOD:
		if(left == V_STRING || right == V_STRING)
			break; // can't do with string
		if(left == V_FLOAT || right == V_FLOAT)
			vartype = V_FLOAT;
		else // int or char or bool or enum
			vartype = V_INT;
		op_result.cast_var = vartype;
		op_result.result_var = vartype;
		op_result.result = OpResult::YES;
		break;
	case S_PLUS:
	case S_MINUS:
		if(left == V_INT || left == V_FLOAT)
			vartype = (CoreVarType)left;
		else if(left == V_BOOL || left == V_CHAR || ltype->IsEnum())
			vartype = V_INT;
		else
			break;
		op_result.cast_var = vartype;
		op_result.result_var = vartype;
		op_result.result = OpResult::YES;
		break;
	case S_EQUAL:
	case S_NOT_EQUAL:
	case S_GREATER:
	case S_GREATER_EQUAL:
	case S_LESS:
	case S_LESS_EQUAL:
		if(left == V_STRING || right == V_STRING)
		{
			if(symbol != S_EQUAL && symbol != S_NOT_EQUAL)
				break;
			vartype = V_STRING;
		}
		else if(left == V_FLOAT || right == V_FLOAT)
			vartype = V_FLOAT;
		else if(left == V_INT || right == V_INT || ltype->IsEnum() || rtype->IsEnum())
			vartype = V_INT;
		else if(symbol == S_EQUAL || symbol == S_NOT_EQUAL)
		{
			if(left == V_CHAR || right == V_CHAR)
				vartype = V_CHAR;
			else
			{
				assert(left == V_BOOL && right == V_BOOL);
				vartype = V_BOOL;
			}
		}
		else
			vartype = V_INT;
		op_result.cast_var = vartype;
		op_result.result_var = V_BOOL;
		op_result.result = OpResult::YES;
		break;
	case S_AND:
	case S_OR:
		// not allowed for string, cast other to bool
		if(left == V_STRING || right == V_STRING)
			break;
		op_result.cast_var = V_BOOL;
		op_result.result_var = V_BOOL;
		op_result.result = OpResult::YES;
		break;
	case S_NOT:
		// not allowed for string, cast other to bool
		if(left == V_STRING)
			break;
		op_result.cast_var = V_BOOL;
		op_result.result_var = V_BOOL;
		op_result.result = OpResult::YES;
		break;
	case S_BIT_AND:
	case S_BIT_OR:
	case S_BIT_XOR:
	case S_BIT_LSHIFT:
	case S_BIT_RSHIFT:
		// not allowed for string, cast other to int
		if(left == V_STRING || right == V_STRING)
			break;
		op_result.cast_var = V_INT;
		op_result.result_var = V_INT;
		op_result.result = OpResult::YES;
		break;
	case S_BIT_NOT:
		// not allowed for string, cast other to int
		if(left == V_STRING)
			break;
		op_result.cast_var = V_INT;
		op_result.result_var = V_INT;
		op_result.result = OpResult::YES;
		break;
	case S_IS:
		if(ltype->IsClass() && left == right)
		{
			op_result.cast_var = VarType(left, 0);
			op_result.result = OpResult::YES;
		}
		else if(leftvar.type == V_REF && rightvar.type == V_REF && leftvar.subtype == rightvar.subtype)
		{
			op_result.cast_var = leftvar;
			op_result.result = OpResult::YES;
		}
		else if(left == V_STRING && right == V_STRING)
		{
			op_result.cast_var = V_STRING;
			op_result.result = OpResult::YES;
		}
		op_result.result_var = V_BOOL;
		break;
	case S_PRE_INC:
	case S_PRE_DEC:
	case S_POST_INC:
	case S_POST_DEC:
		// hardcoded handling in ParseExprApplySymbol (called CanOp for overload checking)
		op_result.result = OpResult::FALLBACK;
		break;
	default:
		assert(0);
		break;
	}

	return op_result;
}

bool Parser::TryConstExpr(ParseNode* left, ParseNode* right, ParseNode* op, SYMBOL symbol)
{
	if(left->op != right->op)
		return false;

	switch(left->op)
	{
	case PUSH_BOOL:
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
			op->pseudo_op = PUSH_BOOL;
		}
		break;
	case PUSH_CHAR:
		{
			// optimize const char expr
			switch(symbol)
			{
			case S_EQUAL:
				op->bvalue = (left->cvalue == right->cvalue);
				op->pseudo_op = PUSH_BOOL;
				break;
			case S_NOT_EQUAL:
				op->bvalue = (left->cvalue != right->cvalue);
				op->pseudo_op = PUSH_BOOL;
				break;
			default:
				assert(0);
				op->cvalue = 0;
				op->op = PUSH_CHAR;
				op->result = V_CHAR;
				break;
			}
		}
		break;
	case PUSH_INT:
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
				op->result = V_INT;
				break;
			}
		}
		break;
	case PUSH_FLOAT:
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
				op->pseudo_op = PUSH_BOOL;
				break;
			case S_NOT_EQUAL:
				op->bvalue = (left->fvalue != right->fvalue);
				op->pseudo_op = PUSH_BOOL;
				break;
			case S_LESS:
				op->bvalue = (left->fvalue < right->fvalue);
				op->pseudo_op = PUSH_BOOL;
				break;
			case S_LESS_EQUAL:
				op->bvalue = (left->fvalue <= right->fvalue);
				op->pseudo_op = PUSH_BOOL;
				break;
			case S_GREATER:
				op->bvalue = (left->fvalue > right->fvalue);
				op->pseudo_op = PUSH_BOOL;
				break;
			case S_GREATER_EQUAL:
				op->bvalue = (left->fvalue >= right->fvalue);
				op->pseudo_op = PUSH_BOOL;
				break;
			default:
				assert(0);
				op->fvalue = 0;
				op->op = PUSH_FLOAT;
				op->result = V_FLOAT;
				break;
			}
		}
		break;
	default:
		return false;
	}

	left->Free();
	right->Free();
	return true;
}

bool Parser::TryConstExpr1(ParseNode* node, SYMBOL symbol)
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

bool Parser::Cast(ParseNode*& node, VarType vartype, int cast_flags, CastResult* _cast_result)
{
	CastResult cast_result;
	if(_cast_result)
		cast_result = *_cast_result;
	else
		cast_result = MayCast(node, vartype, IS_SET(cast_flags, CF_PASS_BY_REF));
	assert(!cast_result.CantCast());

	// no cast required?
	if(!cast_result.NeedCast())
		return true;

	bool implici = !IS_SET(cast_flags, CF_EXPLICIT);
	assert(!implici || IS_SET(cast_result.type, CastResult::IMPLICIT_CAST | CastResult::IMPLICIT_CTOR) || cast_result.type == CastResult::NOT_REQUIRED);

	// can const cast?
	if(DoConstCast(node, vartype))
		return true;
	else if(IS_SET(cast_flags, CF_REQUIRE_CONST))
		return false;

	if(cast_result.ref_type == CastResult::DEREF)
	{
		// dereference
		ParseNode* deref = ParseNode::Get();
		deref->op = DEREF;
		deref->result = VarType(node->result.subtype, 0);
		deref->source = nullptr;
		deref->push(node);
		node = deref;
	}
	else if(cast_result.ref_type == CastResult::TAKE_REF)
	{
		// take address
		assert(node->op == PUSH_LOCAL || node->op == PUSH_GLOBAL || node->op == PUSH_ARG || node->op == PUSH_MEMBER || node->op == PUSH_THIS_MEMBER);
		node->op = Op(node->op + 1);
		node->result = VarType(V_REF, node->result.type);
		if(node->op == PUSH_LOCAL_REF || node->op == PUSH_ARG_REF)
			GetVar(node)->referenced = true;
		if(node->op == PUSH_THIS_MEMBER)
		{
			Member* m = current_type->members[node->value];
			if(m->used == Member::Used)
				m->used = Member::Set;
		}
		if(node->source)
			node->source->mod = true;
	}

	// cast?
	if(IS_SET(cast_result.type, CastResult::IMPLICIT_CAST) || (!implici && IS_SET(cast_result.type, CastResult::EXPLICIT_CAST)))
	{
		ParseNode* cast = ParseNode::Get();
		if(IS_SET(cast_result.type, CastResult::BUILTIN_CAST))
		{
			// builtin type cast
			cast->op = CAST;
			cast->value = vartype.type;
			cast->result = vartype;
		}
		else
		{
			// user defined function cast
			CheckFunctionIsDeleted(*cast_result.cast_func.cf);
			cast->op = (cast_result.cast_func.IsParse() ? CALLU : CALL);
			cast->result = cast_result.cast_func.cf->result;
			cast->value = cast_result.cast_func.cf->index;
		}
		cast->source = nullptr;
		cast->push(node);
		node = cast;
	}
	else if(IS_SET(cast_result.type, CastResult::IMPLICIT_CTOR))
	{
		// ctor cast
		CheckFunctionIsDeleted(*cast_result.ctor_func.cf);
		ParseNode* cast = ParseNode::Get();
		cast->op = (cast_result.ctor_func.IsParse() ? CALLU_CTOR : CALL);
		cast->result = cast_result.ctor_func.cf->result;
		cast->value = cast_result.ctor_func.cf->index;
		cast->source = nullptr;
		cast->push(node);
		node = cast;
	}

	return true;
}

// used in var assignment, passing argument to function
bool Parser::TryCast(ParseNode*& node, VarType vartype, bool implici, bool pass_by_ref)
{
	CastResult c = MayCast(node, vartype, pass_by_ref);
	if(c.CantCast())
		return false;
	else if(c.NeedCast())
	{
		if(implici)
		{
			if(!IS_SET(c.type, CastResult::IMPLICIT_CAST | CastResult::IMPLICIT_CTOR) && c.type != CastResult::NOT_REQUIRED)
				return false;
		}
		Cast(node, vartype, implici ? 0 : CF_EXPLICIT, &c);
	}
	return true;
}

bool Parser::DoConstCast(ParseNode* node, VarType vartype)
{
	// can cast only const literal
	if(vartype == V_STRING)
		return false;

	switch(COMBINE(node->op, vartype.type))
	{
	case COMBINE(PUSH_BOOL, V_CHAR):
		node->cvalue = (node->bvalue ? 't' : 'f');
		node->op = PUSH_CHAR;
		node->result = V_CHAR;
		return true;
	case COMBINE(PUSH_BOOL, V_INT):
		node->value = (node->bvalue ? 1 : 0);
		node->op = PUSH_INT;
		node->result = V_INT;
		return true;
	case COMBINE(PUSH_BOOL, V_FLOAT):
		node->fvalue = (node->bvalue ? 1.f : 0.f);
		node->op = PUSH_FLOAT;
		node->result = V_FLOAT;
		return true;
	case COMBINE(PUSH_CHAR, V_BOOL):
		node->bvalue = (node->cvalue != 0);
		node->pseudo_op = PUSH_BOOL;
		node->result = V_BOOL;
		return true;
	case COMBINE(PUSH_CHAR, V_INT):
		node->value = (int)node->cvalue;
		node->op = PUSH_INT;
		node->result = V_INT;
		return true;
	case COMBINE(PUSH_CHAR, V_FLOAT):
		node->fvalue = (float)node->cvalue;
		node->op = PUSH_FLOAT;
		node->result = V_FLOAT;
		return true;
	case COMBINE(PUSH_INT, V_BOOL):
		node->bvalue = (node->value != 0);
		node->pseudo_op = PUSH_BOOL;
		node->result = V_BOOL;
		return true;
	case COMBINE(PUSH_INT, V_CHAR):
		node->cvalue = (char)node->value;
		node->op = PUSH_CHAR;
		node->result = V_CHAR;
		return true;
	case COMBINE(PUSH_INT, V_FLOAT):
		node->fvalue = (float)node->value;
		node->op = PUSH_FLOAT;
		node->result = V_FLOAT;
		return true;
	case COMBINE(PUSH_FLOAT, V_BOOL):
		node->bvalue = (node->fvalue != 0.f);
		node->pseudo_op = PUSH_BOOL;
		node->result = V_BOOL;
		return true;
	case COMBINE(PUSH_FLOAT, V_CHAR):
		node->cvalue = (char)node->fvalue;
		node->op = PUSH_CHAR;
		node->result = V_CHAR;
		return true;
	case COMBINE(PUSH_FLOAT, V_INT):
		node->value = (int)node->fvalue;
		node->op = PUSH_INT;
		node->result = V_INT;
		return true;
	default:
		{
			bool right_enum = GetType(vartype.type)->IsEnum();
			if((node->op == PUSH_ENUM || node->result.type == V_INT) && (right_enum || vartype.type == V_INT))
			{
				node->result.type = vartype.type;
				node->op = (right_enum ? PUSH_ENUM : PUSH_INT);
				return true;
			}
			else
				return false;
		}
	}
}

CastResult Parser::MayCast(ParseNode* node, VarType vartype, bool pass_by_ref)
{
	CastResult result;

	if(node->result.GetType() == vartype.GetType())
	{
		// types match, no cast required
		result.type = CastResult::NOT_REQUIRED;
	}
	else
	{
		node->result.GetType();
		Type* left = GetType(node->result.type);
		Type* right = GetType(vartype.type);
		bool builtin_cast = false;
		bool left_builtin = (left->IsBuiltin() || (node->result.type == V_REF && GetType(node->result.subtype)->IsBuiltin()));
		bool right_builtin = (right->IsBuiltin() || (vartype.type == V_REF && GetType(vartype.subtype)->IsBuiltin()));

		// cast
		if(left_builtin && right_builtin)
		{
			// builtin cast between bool/char/int/float/string/enum
			// everything is allowed except string to other type
			if(node->result.GetType() != V_STRING)
			{
				builtin_cast = true;
				result.type |= CastResult::IMPLICIT_CAST | CastResult::BUILTIN_CAST;
			}
		}
		else
		{
			// find cast function
			result.cast_func = FindSpecialFunction(left, SF_CAST, [vartype](AnyFunction& f) {return f.cf->result == vartype; });
			if(!result.cast_func && node->result.type == V_REF)
			{
				// find cast function when passed by reference
				result.cast_func = FindSpecialFunction(GetType(node->result.subtype), SF_CAST, [vartype](AnyFunction& f) {return f.cf->result == vartype; });
			}
		}

		// find ctor cast function
		result.ctor_func = FindSpecialFunction(right, SF_CTOR, [node](AnyFunction& f)
		{
			uint required = (f.IsParse() ? 2u : 1u);
			return f.cf->arg_infos.size() == required
				&& f.cf->arg_infos[required - 1].vartype == node->result
				&& f.cf->IsImplicit();
		});
		if(!result.ctor_func && node->result.type == V_REF)
		{
			// find ctor cast function when passed by reference
			result.ctor_func = FindSpecialFunction(right, SF_CTOR, [node](AnyFunction& f)
			{
				uint required = (f.IsParse() ? 2u : 1u);
				return f.cf->arg_infos.size() == required
					&& f.cf->arg_infos[required - 1].vartype.type == node->result.subtype
					&& f.cf->IsImplicit();
			});
		}

		// found anything?
		if(!result.cast_func && !builtin_cast && !result.ctor_func)
		{
			result.type = CastResult::CANT;
			return result;
		}

		// mark cast type
		if(result.cast_func)
		{
			if(result.cast_func.cf->IsImplicit())
				result.type |= CastResult::IMPLICIT_CAST;
			else
				result.type |= CastResult::EXPLICIT_CAST;
			if(result.cast_func.cf->IsBuiltin())
				result.type |= CastResult::BUILTIN_CAST;
		}
		if(result.ctor_func)
			result.type |= CastResult::IMPLICIT_CTOR;
	}

	if(vartype.type != V_REF)
	{
		// require value type
		if(node->result.type == V_REF && !(pass_by_ref && node->source && node->source->index == -1 && ((ReturnStructVar*)node->source)->code_result))
			result.ref_type = CastResult::DEREF; // dereference
	}
	else
	{
		// require reference to value type, cast not allowed
		if(!CanTakeRef(node))
			result.type = CastResult::CANT; // can't take address
		else
		{
			if(result.type != CastResult::NOT_REQUIRED)
				result.type = CastResult::CANT; // can't take address, type mismatch
			else if(node->result.type != V_REF)
				result.ref_type = CastResult::TAKE_REF; // take address
		}
	}

	return result;
}

void Parser::ForceCast(ParseNode*& node, VarType vartype, cstring op)
{
	if(!TryCast(node, vartype))
		t.Throw("Can't cast return value from '%s' to '%s' for operation '%s'.", GetTypeName(node), GetName(vartype), op);
}

bool Parser::TryConstCast(ParseNode*& node, VarType type)
{
	CastResult c = MayCast(node, type, false);
	if(c.CantCast())
		return false;
	else if(c.NeedCast())
		return Cast(node, type, CF_REQUIRE_CONST);
	else
		return true;
}

Op Parser::PushToSet(ParseNode* node)
{
	switch(node->op)
	{
	case PUSH_LOCAL:
		return SET_LOCAL;
	case PUSH_GLOBAL:
		return SET_GLOBAL;
	case PUSH_ARG:
		return SET_ARG;
	case PUSH_MEMBER:
		return SET_MEMBER;
	case PUSH_THIS_MEMBER:
		{
			Member* m = current_type->members[node->value];
			if(m->used == Member::Used)
				m->used = Member::Set;
			return SET_THIS_MEMBER;
		}
	default:
		return (Op)NOP;
	}
}

bool Parser::CanTakeRef(ParseNode* node, bool allow_ref)
{
	return node->op == PUSH_GLOBAL
		|| node->op == PUSH_LOCAL
		|| node->op == PUSH_ARG
		|| node->op == PUSH_MEMBER
		|| node->op == PUSH_THIS_MEMBER
		|| (allow_ref && node->result.type == V_REF);
}

void Parser::Optimize()
{
	if(optimize)
	{
		for(ParseFunction* ufunc : ufuncs)
		{
			if(ufunc->node)
				OptimizeTree(ufunc->node);
		}
		OptimizeTree(global_node);
	}
}

// childs can be nullptr only for if/while
ParseNode* Parser::OptimizeTree(ParseNode* node)
{
	assert(node);

	if(node->pseudo_op == IF || node->pseudo_op == TERNARY)
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
				ParseNode * not = ParseNode::Get();
				not->op = NOT;
				not->result = V_BOOL;
				not->source = nullptr;
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
		LoopAndRemove(node->childs, [this](ParseNode*& node) {
			node = OptimizeTree(node);
			return !node;
		});
	}

	return node;
}

void Parser::CheckReturnValues()
{
	for(ParseFunction* ufunc : ufuncs)
		VerifyFunctionReturnValue(ufunc);
}

void Parser::VerifyFunctionReturnValue(ParseFunction* f)
{
	if(f->result.type == V_VOID || f->special == SF_CTOR)
		return;

	if(f->node)
	{
		if(f->node->pseudo_op == RETURN || f->node->pseudo_op == INTERNAL_GROUP)
			return;

		for(vector<ParseNode*>::reverse_iterator it = f->node->childs.rbegin(), end = f->node->childs.rend(); it != end; ++it)
		{
			if(VerifyNodeReturnValue(*it, false) == RI_YES)
				return;
		}
	}
	else if(f->IsBuiltin())
		return;

	t.Throw("%s '%s' not always return value.", f->type == V_VOID ? "Function" : "Method", GetName(f));
}

RETURN_INFO Parser::VerifyNodeReturnValue(ParseNode* node, bool in_switch)
{
	switch(node->pseudo_op)
	{
	case GROUP:
	case CASE_BLOCK:
		for(auto it = node->childs.begin(), end = node->childs.end(); it != end; ++it)
		{
			RETURN_INFO ri = VerifyNodeReturnValue(*it, in_switch);
			if(ri == RI_YES || (in_switch && ri == RI_BREAK))
				return ri;
		}
		return RI_NO;
	case IF:
		if(node->childs.size() == 3u && node->childs[1] && node->childs[2])
		{
			RETURN_INFO ri1 = VerifyNodeReturnValue(node->childs[1], in_switch),
				ri2 = VerifyNodeReturnValue(node->childs[2], in_switch);
			if(ri1 == ri2 && (ri1 == RI_YES || (ri1 == RI_BREAK && in_switch)))
				return ri1;
		}
		return RI_NO;
	case RETURN:
		return RI_YES;
	case BREAK:
		return (in_switch ? RI_BREAK : RI_NO);
	case SWITCH:
		{
			bool have_def = false;
			int blocks = 0;
			for(auto it = node->childs.begin() + 1, end = node->childs.end(); it != end; ++it)
			{
				ParseNode* cas = *it;
				if(cas->pseudo_op == DEFAULT_CASE)
				{
					++blocks;
					have_def = true;
					continue;
				}
				else if(cas->pseudo_op == CASE)
				{
					++blocks;
					continue;
				}
				assert(cas->pseudo_op == CASE_BLOCK);
				RETURN_INFO ri = VerifyNodeReturnValue(cas, true);
				if(ri == RI_YES)
				{
					blocks = 0;
					continue;
				}
				else if(ri == RI_BREAK)
					return RI_NO;
			}
			return ((blocks == 0 && have_def) ? RI_YES : RI_NO);
		}
	default:
		return RI_NO;
	}
}

void Parser::CopyFunctionChangedStructs()
{
	for(ParseFunction* f : ufuncs)
	{
		ParseNode* node = nullptr;
		for(ParseVar* local : f->args)
		{
			if(local->mod && GetType(local->vartype.type)->IsStruct())
			{
				if(!node)
				{
					node = ParseNode::Get();
					node->pseudo_op = INTERNAL_GROUP;
					node->result = V_VOID;
					node->source = nullptr;
				}
				node->push(COPY_ARG, local->index);
			}
		}
		if(node)
			f->node->childs.insert(f->node->childs.begin(), node);
	}
}

void Parser::ConvertToBytecode()
{
	uint jmp_to_next_section = 0;
	bool first_script = module->code.empty();
	if(!first_script)
	{
		module->code.pop_back();
		module->code.push_back(JMP);
		jmp_to_next_section = module->code.size();
		module->code.push_back(0);
	}

	for(ParseFunction* ufunc : ufuncs)
	{
		if(ufunc->IsBuiltin())
			continue;
		ufunc->pos = module->code.size();
		ufunc->locals = (ufunc->block ? ufunc->block->GetMaxVars() : 0);
		uint old_size = module->code.size();
		if(ufunc->node)
			ToCode(module->code, ufunc->node, nullptr);
		if(old_size == module->code.size() || module->code.back() != RET)
			module->code.push_back(RET);
	}

	if(first_script)
		module->entry_point = module->code.size();
	else
		module->code[jmp_to_next_section] = module->code.size();
	uint old_size = module->code.size();
	ToCode(module->code, global_node, nullptr);
	module->code.push_back(RET);
}

void Parser::ToCode(vector<int>& code, ParseNode* node, vector<uint>* break_pos)
{
	if(node->pseudo_op >= SPECIAL_OP)
	{
		switch(node->pseudo_op)
		{
		case IF:
		case TERNARY:
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
						if(node->childs[1]->result != V_VOID && node->pseudo_op != TERNARY)
							code.push_back(POP);
					}
					code.push_back(JMP);
					uint jmp_pos = code.size();
					code.push_back(0);
					uint else_start = code.size();
					if(node->childs[2])
					{
						ToCode(code, node->childs[2], break_pos);
						if(node->childs[2]->result != V_VOID && node->pseudo_op != TERNARY)
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
						if(node->childs[1]->result != V_VOID && node->pseudo_op != TERNARY)
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
					if(for1->result != V_VOID)
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
					if(block->result != V_VOID)
						code.push_back(POP);
				}
				if(for3)
				{
					ToCode(code, for3, &wh_break_pos);
					if(for3->result != V_VOID)
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
		case CASE_BLOCK:
			{
				for(ParseNode* n : node->childs)
				{
					ToCode(code, n, break_pos);
					if(n->result != V_VOID)
						code.push_back(POP);
				}
			}
			break;
		case SWITCH:
			{
				// push switch value
				ToCode(code, node->childs[0], break_pos);
				code.push_back(SET_TMP);
				code.push_back(POP);

				vector<uint> wh_break_pos;
				struct Jmp
				{
					uint pos;
					ParseNode* link;
				};
				vector<Jmp> jmps;

				// jmp blocks
				ParseNode* def_link = nullptr;
				for(auto it = node->childs.begin() + 1, end = node->childs.end(); it != end; ++it)
				{
					ParseNode* cas = *it;
					if(cas->pseudo_op == CASE_BLOCK)
						continue;
					if(cas->pseudo_op == DEFAULT_CASE)
					{
						def_link = cas;
						continue;
					}
					code.push_back(PUSH_TMP);
					switch(cas->result.type)
					{
					case V_BOOL:
						code.push_back(cas->bvalue ? PUSH_TRUE : PUSH_FALSE);
						break;
					case V_CHAR:
						code.push_back(PUSH_CHAR);
						code.push_back(union_cast<int>(cas->cvalue));
						break;
					case V_INT:
						code.push_back(PUSH_INT);
						code.push_back(cas->value);
						break;
					case V_FLOAT:
						code.push_back(PUSH_FLOAT);
						code.push_back(union_cast<int>(cas->fvalue));
						break;
					case V_STRING:
						code.push_back(PUSH_STRING);
						code.push_back(cas->value);
						break;
					default:
						assert(GetType(cas->result.type)->IsEnum());
						code.push_back(PUSH_ENUM);
						code.push_back(cas->result.type);
						code.push_back(cas->value);
						break;
					}
					code.push_back(EQ);
					code.push_back(TJMP);
					Jmp j = { code.size(), cas->linked };
					jmps.push_back(j);
					code.push_back(0);
				}

				// end jmp
				code.push_back(JMP);
				if(def_link)
				{
					Jmp j = { code.size(), def_link->linked };
					jmps.push_back(j);
				}
				else
					wh_break_pos.push_back(code.size());
				code.push_back(0);

				// blocks
				for(auto it = node->childs.begin() + 1, end = node->childs.end(); it != end; ++it)
				{
					ParseNode* blk = *it;
					if(blk->pseudo_op != CASE_BLOCK)
						continue;
					blk->value = code.size();
					ToCode(code, blk, &wh_break_pos);
				}

				// patch jmps
				uint end_pos = code.size();
				for(Jmp& j : jmps)
				{
					if(j.link)
						code[j.pos] = j.link->value;
					else
						code[j.pos] = end_pos;
				}
				for(uint p : wh_break_pos)
					code[p] = end_pos;
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
	case CALL:
	case CALLU:
	case CALLU_CTOR:
		{
			code.push_back(node->op);
			int f_idx = node->value;
			if(node->op != CALL)
				f_idx += ufunc_offset;
			code.push_back(f_idx);
			if(node->op != CALLU_CTOR && node->source && node->source->mod && node->source->index == -1 && !((ReturnStructVar*)node->source)->code_result)
				code.push_back(COPY);
		}
		break;
	case PUSH_INT:
	case PUSH_STRING:
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
	case COPY_ARG:
	case SWAP:
	case RELEASE_REF:
	case RELEASE_OBJ:
	case LINE:
		code.push_back(node->op);
		code.push_back(node->value);
		break;
	case PUSH_CHAR:
		code.push_back(node->op);
		code.push_back(union_cast<int>(node->cvalue));
		break;
	case PUSH_FLOAT:
		code.push_back(node->op);
		code.push_back(union_cast<int>(node->fvalue));
		break;
	case PUSH:
	case PUSH_TMP:
	case SET_TMP:
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
	case COPY:
	case PUSH_INDEX:
	case PUSH_THIS:
	case RET:
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
	case PUSH_ENUM:
		code.push_back(PUSH_ENUM);
		code.push_back(node->result.type);
		code.push_back(node->value);
		break;
	default:
		assert(0);
		break;
	}
}

Type* Parser::GetType(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int type_index = (index & 0xFFFF);
	if(module_index == 0xFFFF)
	{
		assert(type_index < (int)module->tmp_types.size());
		return module->tmp_types[type_index];
	}
	else
	{
		assert(module->modules.find(module_index) != module->modules.end());
		Module* m = module->modules[module_index];
		assert(type_index < (int)m->types.size());
		return m->types[type_index];
	}
}

Function* Parser::GetFunction(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int func_index = (index & 0xFFFF);
	assert(module->modules.find(module_index) != module->modules.end());
	Module* m = module->modules[module_index];
	assert(func_index < (int)m->functions.size());
	return m->functions[func_index];
}

VarType Parser::GetReturnType(ParseNode* node)
{
	if(node->childs.empty())
		return V_VOID;
	else
		return node->childs.front()->result;
}

cstring Parser::GetName(ParseVar* var)
{
	assert(var);
	return Format("%s %s", GetName(var->vartype), var->name.c_str());
}

cstring Parser::GetName(CommonFunction* cf, bool write_result, bool write_type, BASIC_SYMBOL* symbol)
{
	assert(cf);
	LocalString s = "";
	if(write_result && cf->special != SF_CTOR && cf->special != SF_DTOR)
	{
		s += GetName(cf->result);
		s += ' ';
	}
	uint var_offset = 0;
	if(cf->type != V_VOID)
	{
		if(write_type)
		{
			s += GetType(cf->type)->name;
			s += '.';
		}
		if(cf->PassThis())
			++var_offset;
	}
	if(!symbol)
	{
		if(cf->name[0] == '$')
		{
			if(cf->name == "$opCast")
				s += "operator cast";
			else if(cf->name == "$opAddref")
				s += "operator addref";
			else if(cf->name == "$opRelease")
				s += "operator release";
			else
			{
				for(int i = 0; i < S_MAX; ++i)
				{
					SymbolInfo& si = symbols[i];
					if(si.op_code && strcmp(si.op_code, cf->name.c_str()) == 0)
					{
						s += "operator ";
						s += si.oper;
						s += ' ';
						break;
					}
				}
			}
		}
		else
			s += cf->name;
	}
	else
	{
		s += "operator ";
		s += basic_symbols[*symbol].GetOverloadText();
		s += ' ';
	}
	s += '(';
	for(uint i = var_offset, count = cf->arg_infos.size(); i < count; ++i)
	{
		if(i != var_offset)
			s += ',';
		s += GetName(cf->arg_infos[i].vartype);
		if(cf->arg_infos[i].pass_by_ref)
			s += '&';
	}
	s += ')';
	return Format("%s", s->c_str());
}

cstring Parser::GetName(VarType vartype)
{
	Type* t = GetType(vartype.type == V_REF ? vartype.subtype : vartype.type);
	if(vartype.type != V_REF)
		return t->name.c_str();
	else
		return Format("%s&", t->name.c_str());
}

cstring Parser::GetTypeName(ParseNode* node)
{
	return GetName(node->result);
}

cstring Parser::GetParserFunctionName(uint index)
{
	assert(index < ufuncs.size());
	return GetName(ufuncs[index], false);
}

VarType Parser::CommonType(VarType a, VarType b)
{
	if(a == b)
		return a;
	if(a.type == V_VOID || b.type == V_VOID)
		return (CoreVarType)-1;
	if(a.type == V_FLOAT || b.type == V_FLOAT)
		return V_FLOAT;
	else if(a.type == V_INT || b.type == V_INT)
		return V_INT;
	else if(a.type == V_CHAR || b.type == V_CHAR)
		return V_CHAR;
	else
	{
		// bool && bool only left but is used by first if
		assert(0);
		return (CoreVarType)-1;
	}
}

FOUND Parser::FindItem(const string& id, Found& found)
{
	if(current_type)
	{
		found.member = current_type->FindMember(id, found.member_index);
		if(found.member)
			return F_MEMBER;

		AnyFunction f = FindFunction(current_type, id);
		if(f)
		{
			if(f.IsParse())
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

	Function* func = FindFunction(id);
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

Function* Parser::FindFunction(const string& name)
{
	for(auto& m : module->modules)
	{
		for(Function* f : m.second->functions)
		{
			if(f->name == name && f->type == V_VOID)
				return f;
		}
	}
	return nullptr;
}

AnyFunction Parser::FindFunction(Type* type, const string& name)
{
	assert(type);

	for(Function* f : type->funcs)
	{
		if(f->name == name)
			return f;
	}

	for(ParseFunction* pf : type->ufuncs)
	{
		if(pf->name == name)
			return pf;
	}

	return nullptr;
}

void Parser::FindAllFunctionOverloads(const string& name, vector<AnyFunction>& items)
{
	for(auto& m : module->modules)
	{
		for(Function* f : m.second->functions)
		{
			if(f->name == name && f->type == V_VOID)
				items.push_back(f);
		}
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

void Parser::FindAllFunctionOverloads(Type* type, const string& name, vector<AnyFunction>& funcs)
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

void Parser::FindAllStaticFunctionOverloads(Type* type, const string& name, vector<AnyFunction>& funcs)
{
	for(Function* f : type->funcs)
	{
		if(f->name == name && f->IsStatic())
			funcs.push_back(f);
	}

	for(ParseFunction* pf : type->ufuncs)
	{
		if(pf->name == name && pf->IsStatic())
			funcs.push_back(pf);
	}
}

void Parser::FindAllCtors(Type* type, vector<AnyFunction>& funcs)
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

AnyFunction Parser::FindFunction(Type* type, cstring name, delegate<bool(AnyFunction& f)> pred)
{
	for(Function* f : type->funcs)
	{
		if(f->name == name && pred(AnyFunction(f)))
			return f;
	}

	for(ParseFunction* pf : type->ufuncs)
	{
		if(pf->name == name && pred(AnyFunction(pf)))
			return pf;
	}

	return nullptr;
}

AnyFunction Parser::FindSpecialFunction(Type* type, SpecialFunction spec, delegate<bool(AnyFunction& f)> pred)
{
	for(Function* f : type->funcs)
	{
		if(f->special == spec && pred(AnyFunction(f)))
			return f;
	}

	for(ParseFunction* pf : type->ufuncs)
	{
		if(pf->special == spec && pred(AnyFunction(pf)))
			return pf;
	}

	return nullptr;
}

AnyFunction Parser::FindEqualFunction(ParseFunction* pf)
{
	for(auto& m : module->modules)
	{
		for(Function* f : m.second->functions)
		{
			if(f->name == pf->name && f->Equal(*pf))
				return f;
		}
	}

	for(ParseFunction* f : ufuncs)
	{
		if(f->name == pf->name && f->Equal(*pf))
			return f;
	}

	return nullptr;
}

AnyFunction Parser::FindEqualFunction(Type* type, AnyFunction _f)
{
	assert(type);
	CommonFunction& cf = *_f.cf;

	if(cf.special == SF_NO || cf.special == SF_CTOR)
	{
		for(Function* f : type->funcs)
		{
			if(f->name == cf.name && f->Equal(cf))
				return f;
		}

		for(ParseFunction* pf : type->ufuncs)
		{
			if(pf->name == cf.name && pf->Equal(cf))
				return pf;
		}
	}
	else if(cf.special != SF_CAST)
	{
		for(Function* f : type->funcs)
		{
			if(f->special == cf.special)
				return f;
		}

		for(ParseFunction* pf : type->ufuncs)
		{
			if(pf->special == cf.special)
				return pf;
		}
	}
	else
	{
		assert(cf.special == SF_CAST);
		for(Function* f : type->funcs)
		{
			if(f->special == cf.special && f->result == cf.result)
				return f;
		}

		for(ParseFunction* pf : type->ufuncs)
		{
			if(pf->special == cf.special && pf->result == cf.result)
				return pf;
		}
	}

	return nullptr;
}

VarType Parser::GetRequiredType(ParseNode* node, ArgInfo& arg)
{
	VarType required_type = arg.vartype;
	if(arg.pass_by_ref && required_type.type == V_STRING && node->result.type == V_STRING && node->source && node->source->is_code_class)
	{
		required_type.subtype = required_type.type;
		required_type.type = V_REF;
	}
	return required_type;
}

// 0 - don't match, 1 - require cast, 2 - require deref/take address, 3 - match
int Parser::MatchFunctionCall(ParseNode* node, CommonFunction& f, bool is_parse, bool obj_call)
{
	bool is_static = f.IsStatic();
	uint offset = 0, node_offset = 0;
	if((current_type && f.type == current_type->index && !is_static) // this call
		|| (f.special == SF_CTOR && is_parse)) // ctor
		++offset;
	else if(is_static && obj_call) // static call on type
		++node_offset;

	if(node->childs.size() + offset - node_offset > f.arg_infos.size() || node->childs.size() + offset < f.required_args)
		return 0;

	bool require_cast = false;
	bool require_ref = false;
	for(uint i = node_offset; i < node->childs.size(); ++i)
	{
		ArgInfo& arg = f.arg_infos[i + offset - node_offset];
		ParseNode* child = node->childs[i];
		VarType required_type = GetRequiredType(child, arg);
		CastResult c = MayCast(child, required_type, arg.pass_by_ref);
		if(c.CantCast())
			return 0;
		else
		{
			if(c.type != CastResult::NOT_REQUIRED)
				require_cast = true;
			if(c.ref_type != CastResult::NO)
				require_ref = true;
		}
	}

	if(require_cast)
		return 1;
	else if(require_ref)
		return 2;
	else
		return 3;
}

// return function if it's builtin
AnyFunction Parser::ApplyFunctionCall(ParseNode* node, vector<AnyFunction>& funcs, Type* type, bool ctor, bool obj_call)
{
	assert(!funcs.empty());

	int match_level = 0;
	vector<AnyFunction> match;

	for(AnyFunction& f : funcs)
	{
		int m = MatchFunctionCall(node, *f.f, f.IsParse(), obj_call);
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
		{
			cstring type_name;
			if(type->index == V_TYPE && node->childs.front()->pseudo_op == PUSH_TYPE)
				type_name = GetType(node->childs.front()->value)->name.c_str();
			else
				type_name = type->name.c_str();
			s += Format("method '%s.", type_name);
		}
		else
			s += "function '";
		cstring first = match.front().f->name.c_str();
		if(first[0] == '$')
		{
			for(int i = 0; i < S_MAX; ++i)
			{
				SymbolInfo& si = symbols[i];
				if(si.op_code && strcmp(si.op_code, first) == 0)
				{
					s += "operator ";
					s += si.oper;
					break;
				}
			}
		}
		else
			s += first;
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
				s += GetName(node->childs[i]->result);
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
				s += GetName(f.cf);
			}
		}
		else
			s += Format(" '%s'.", GetName(match.front().cf));
		t.Throw(s->c_str());
	}
	else
	{
		AnyFunction f = match.front();
		CommonFunction& cf = *f.f;
		CheckFunctionIsDeleted(cf);
		if(cf.IsBuiltin())
			return f;
		bool callu_ctor = false;

		// remove push type from static method call or add pop
		if(cf.IsStatic() && obj_call)
		{
			if(!HasSideEffects(node->childs.front()))
			{
				node->childs.front()->Free();
				node->childs.erase(node->childs.begin());
			}
			else
			{
				ParseNode* pop = ParseNode::Get();
				pop->op = POP;
				node->childs.insert(node->childs.begin() + 1, pop);
			}
		}

		if(cf.special == SF_CTOR && f.IsParse())
		{
			// user constructor call
			callu_ctor = true;
		}
		else if(current_type && cf.type == current_type->index && !cf.IsStatic())
		{
			// push this
			ParseNode* thi = ParseNode::Get();
			thi->op = PUSH_ARG;
			thi->result = (CoreVarType)cf.type;
			thi->value = 0;
			thi->source = nullptr;
			node->childs.insert(node->childs.begin(), thi);
		}

		// cast params
		if(match_level != 3)
		{
			for(uint i = 0; i < node->childs.size(); ++i)
			{
				ArgInfo& arg = cf.arg_infos[i];
				ParseNode*& child = node->childs[i];
				VarType required_type = GetRequiredType(child, arg);
				Cast(child, required_type, arg.pass_by_ref ? CF_PASS_BY_REF : 0);
			}
		}

		// fill default params
		for(uint i = node->childs.size() + (callu_ctor ? 1 : 0); i < cf.arg_infos.size(); ++i)
		{
			ArgInfo& arg = cf.arg_infos[i];
			ParseNode* n = ParseNode::Get();
			n->result = arg.vartype;
			n->source = nullptr;
			switch(arg.vartype.type)
			{
			case V_BOOL:
				n->pseudo_op = PUSH_BOOL;
				n->bvalue = arg.bvalue;
				break;
			case V_CHAR:
				n->op = PUSH_CHAR;
				n->cvalue = arg.cvalue;
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
				assert(GetType(arg.vartype.type)->IsEnum());
				n->op = PUSH_ENUM;
				n->value = arg.value;
				break;
			}
			node->push(n);
		}

		// apply type
		node->op = (f.IsParse() ? (callu_ctor ? CALLU_CTOR : CALLU) : CALL);
		node->result = cf.result;
		node->value = cf.index;

		if(GetType(node->result.type)->IsStruct())
		{
			ReturnStructVar* rsv = new ReturnStructVar;
			rsv->index = -1;
			rsv->node = node;
			rsv->mod = false;
			rsv->code_result = false;
			rsvs.push_back(rsv);
			node->source = rsv;
		}
		else if(node->result.type == V_REF && cf.IsCode())
		{
			ReturnStructVar* rsv = new ReturnStructVar;
			rsv->index = -1;
			rsv->node = node;
			rsv->mod = false;
			rsv->code_result = true;
			rsvs.push_back(rsv);
			node->source = rsv;
		}
	}

	return nullptr;
}

void Parser::CheckFunctionIsDeleted(CommonFunction& cf)
{
	if(cf.IsDeleted())
		t.Throw("Can't call '%s', %s marked as deleted.", GetName(&cf), cf.type != V_VOID ? "method" : "function");
}

bool Parser::CanOverload(BASIC_SYMBOL symbol)
{
	BasicSymbolInfo& bsi = basic_symbols[symbol];
	for(int i = 0; i < 3; ++i)
	{
		SYMBOL s = bsi[i];
		if(s != S_INVALID)
		{
			if(symbols[s].op_code)
				return true;
		}
	}
	return false;
}

bool Parser::FindMatchingOverload(CommonFunction& f, BASIC_SYMBOL symbol)
{
	BasicSymbolInfo& bsi = basic_symbols[symbol];
	for(int i = 0; i < 3; ++i)
	{
		SYMBOL s = bsi[i];
		if(s != S_INVALID)
		{
			SymbolInfo& si = symbols[s];
			if(!si.op_code)
				continue;
			if(si.args == f.arg_infos.size() || s == S_SUBSCRIPT || s == S_CALL)
			{
				f.name = si.op_code;
				return true;
			}
		}
	}
	return false;
}

NextType Parser::GetNextType(bool analyze)
{
	// function modifiers
	if(t.IsKeywordGroup(G_KEYWORD))
	{
		int id = t.GetKeywordId(G_KEYWORD);
		if(id == K_IMPLICIT || id == K_DELETE || id == K_STATIC)
			return NT_FUNC;
		else
			return NT_INVALID;
	}

	tokenizer::Pos pos = t.GetPos();

	// dtor
	if(t.IsSymbol('~'))
		t.Next();

	// var type
	if(analyze)
	{
		if(!t.IsKeywordGroup(G_VAR) && !t.IsItem())
		{
			t.MoveTo(pos);
			return NT_INVALID;
		}
		t.Next();
	}
	else
		GetVarType(false);

	NextType result;

	if(t.IsSymbol('('))
	{
		// ctor or dtor decl
		// [~] item/type (
		t.ForceMoveToClosingSymbol('(', ')');
		t.Next();
		if(t.IsSymbol('{'))
			result = NT_FUNC;
		else
			result = NT_CALL;
	}
	else
	{
		if(t.IsSymbol('&'))
			t.Next();

		if(t.IsKeyword(K_OPERATOR, G_KEYWORD))
			result = NT_FUNC;
		else if(!t.IsItem())
			result = NT_INVALID;
		else
		{
			t.Next();
			if(t.IsSymbol('('))
			{
				t.ForceMoveToClosingSymbol('(', ')');
				t.Next();
				if(t.IsSymbol('{'))
					result = NT_FUNC;
				else
					result = NT_VAR_DECL;
			}
			else
				result = NT_VAR_DECL;
		}
	}

	t.MoveTo(pos);
	return result;
}

void Parser::FreeTmpStr(string* str)
{
	RemoveElement(tmp_strs, str);
	StringPool.Free(str);
}

bool Parser::IsCtor(ParseNode* node)
{
	if(node->op == CALLU_CTOR || node->op == COPY)
		return true;
	else if(node->op == CALL)
	{
		Function* f = GetFunction(node->value);
		return f->special == SF_CTOR;
	}
	else
		return false;
}

int Parser::GetEmptyString()
{
	if(empty_string == -1)
	{
		empty_string = module->strs.size();
		Str* str = Str::Get();
		str->s = "";
		str->refs = 1;
		module->strs.push_back(str);
	}
	return empty_string;
}

void Parser::AnalyzeCode()
{
	string str;
	char c;

	while(!t.IsEof())
	{
		if(t.IsKeyword(K_CLASS, G_KEYWORD) || t.IsKeyword(K_STRUCT, G_KEYWORD))
			AnalyzeClass();
		else if(t.IsKeyword(K_ENUM, G_KEYWORD))
			ParseEnum(true);
		else if(t.IsSymbol("([{", &c))
		{
			t.ForceMoveToClosingSymbol(c);
			t.Next();
		}
		else
			AnalyzeLine(nullptr);
	}

	VerifyAllTypes();
	VerifyFunctionsDefaultArguments();

	t.Reset();
}

void Parser::AnalyzeClass()
{
	bool is_class = (t.GetKeywordId(G_KEYWORD) == (int)K_CLASS);
	t.Next();

	Type* type;
	if(t.IsItem())
		type = AnalyzeAddType(t.GetItem());
	else if(t.IsKeywordGroup(G_VAR))
	{
		type = GetType(t.GetKeywordId(G_VAR));
		if(type->declared)
			t.Throw("Can't declare %s '%s', type is already declared.", is_class ? "class" : "struct", t.GetTokenString().c_str());
	}
	else
		t.Unexpected(tokenizer::T_ITEM);

	type->declared = true;
	type->flags = Type::Class;
	if(is_class)
		type->flags |= Type::Ref;
	else
		type->flags |= Type::PassByValue;
	type->SetGenericType();

	t.Next();
	t.AssertSymbol('{');
	t.Next();
	if(!t.IsSymbol('}'))
	{
		while(true)
		{
			AnalyzeLine(type);
			if(t.IsSymbol('}'))
			{
				t.Next();
				break;
			}
		}
	}

	CreateDefaultFunctions(type, type->have_def_value ? 1 : -1);
}

void Parser::AnalyzeLine(Type* type)
{
	NextType next = GetNextType(true);
	if(next == NT_FUNC)
	{
		int flags = 0;
		bool dtor = false;

		// function modifiers
		ParseFuncModifiers(type != nullptr, flags);

		if(t.IsSymbol('~'))
		{
			dtor = true;
			t.Next();
		}

		VarType vartype = AnalyzeVarType();

		if(t.IsSymbol('('))
		{
			assert(vartype.type != V_REF);

			// ctor/dtor
			if(!type)
				t.Throw("%s can only be declared inside class.", dtor ? "Destructor" : "Constructor");

			if(dtor)
			{
				if(IS_SET(flags, CommonFunction::F_STATIC))
					t.Throw("Static destructor not allowed.");
				AnalyzeArgs(V_VOID, SF_DTOR, type, Format("~%s", type->name.c_str()), flags);
			}
			else
			{
				if(IS_SET(flags, CommonFunction::F_STATIC))
					t.Throw("Static constructor not allowed.");
				AnalyzeArgs(vartype, SF_CTOR, type, type->name.c_str(), flags);
			}
		}
		else
		{
			if(dtor)
				t.Throw("Destructor tag mismatch.");

			if(t.IsKeyword(K_OPERATOR, G_KEYWORD))
			{
				// operator
				if(!type)
					t.Throw("Operator function can be used only inside class.");
				if(IS_SET(flags, CommonFunction::F_STATIC))
					t.Throw("Static operator not allowed.");
				t.Next();
				if(t.IsItem())
				{
					// operator string
					const string& item = t.GetItem();
					if(item == "cast")
					{
						t.Next();
						AnalyzeArgs(vartype, SF_CAST, type, "$opCast", flags);
					}
					else if(item == "addref" || item == "release")
						t.Throw("Operator function '%s' can only be registered in code.", item.c_str());
					else
						t.ThrowExpecting("operator");
				}
				else
				{
					// operator symbol
					BASIC_SYMBOL symbol = GetSymbol(true);
					if(symbol == BS_MAX)
						t.ThrowExpecting("symbol");
					if(!CanOverload(symbol))
						t.Throw("Can't overload operator '%s'.", basic_symbols[symbol].GetOverloadText());
					t.Next();
					ParseFunction* func = AnalyzeArgs(vartype, SF_NO, type, "$tmp", flags);
					if(!FindMatchingOverload(*func, symbol))
						t.Throw("Invalid overload operator definition '%s'.", GetName(func, true, true, &symbol));
				}
			}
			else
			{
				// normal function
				static string func_name;
				func_name = t.MustGetItem();
				t.Next();

				t.AssertSymbol('(');
				AnalyzeArgs(vartype, SF_NO, type, func_name.c_str(), flags);
			}
		}
	}
	else if(next == NT_VAR_DECL)
	{
		// var decl
		if(!type)
		{
			t.Next();
			return;
		}

		VarType vartype = AnalyzeVarType();

		do
		{
			Member* m = new Member;
			m->vartype = vartype;
			m->name = t.MustGetItem();
			int index;
			if(type->FindMember(m->name, index))
				t.Throw("Member with name '%s.%s' already exists.", type->name.c_str(), m->name.c_str());
			t.Next();
			m->index = type->members.size();
			m->have_def_value = true;
			type->members.push_back(m);

			if(t.IsSymbol('('))
			{
				m->have_def_value = true;
				m->pos = t.GetPos();
				type->have_def_value = true;
				t.ForceMoveToClosingSymbol('(', ')');
				t.Next();
			}
			else if(t.IsSymbol('='))
			{
				m->have_def_value = true;
				m->pos = t.GetPos();
				type->have_def_value = true;
				if(!t.SkipTo([](Tokenizer& t)
				{
					if(t.IsSymbol())
					{
						switch(t.GetSymbol())
						{
						case '(':
						case '{':
						case '[':
							t.ForceMoveToClosingSymbol(t.GetSymbol());
							break;
						case ',':
						case ';':
							return true;
						}
					}
					return false;
				}))
					t.Throw("Broken member declaration, can't find end.");
			}
			else
				m->have_def_value = false;

			if(t.IsSymbol(';'))
				break;
			t.AssertSymbol(',');
			t.Next();
		} while(1);
	}
	else
	{
		t.Next();
		return;
	}
}

ParseFunction* Parser::AnalyzeArgs(VarType result, SpecialFunction special, Type* type, cstring name, int flags)
{
	Ptr<ParseFunction> func;
	func->result = result;
	func->type = (type ? type->index : V_VOID);
	func->flags = flags;
	func->name = name;
	func->index = ufuncs.size();
	func->required_args = 0;
	func->special = special;
	if(type && !IS_SET(flags, CommonFunction::F_STATIC))
	{
		func->arg_infos.push_back(ArgInfo(VarType(type->index, 0), 0, false));
		func->required_args++;
	}

	// (
	t.Next();
	func->start_pos = t.GetPos();

	if(!t.IsSymbol(')'))
	{
		bool prev_def = false;
		while(true)
		{
			VarType vartype = AnalyzeVarType();

			ParseVar* arg = ParseVar::Get();
			arg->name = t.MustGetItem();
			arg->vartype = vartype;
			arg->subtype = ParseVar::ARG;
			arg->index = func->args.size();
			arg->mod = false;
			arg->is_code_class = GetType(vartype.type)->IsCode();
			func->args.push_back(arg);
			t.Next();
			
			if(t.IsSymbol('='))
			{
				prev_def = true;
				t.Next();
				func->arg_infos.push_back(ArgInfo(vartype, 0, true));
				t.SkipToSymbol("),");
			}
			else
			{
				func->arg_infos.push_back(ArgInfo(vartype, 0, false));
				func->required_args++;
				if(prev_def)
					t.Throw("Missing default value for argument %u '%s %s'.", func->arg_infos.size(), GetName(vartype), arg->name.c_str());
			}
			if(t.IsSymbol(')'))
				break;
			t.AssertSymbol(',');
			t.Next();
		}
	}
	t.Next();

	if(!IS_SET(flags, CommonFunction::F_DELETE))
	{
		t.AssertSymbol('{');
		if(!t.MoveToClosingSymbol('{', '}'))
			t.Throw("Missing closing '}' for function '%s' declaration.", func->name.c_str());
		t.Next();
	}

	if(special == SF_DTOR)
	{
		if(func->arg_infos.size() != 1u)
			t.Throw("Destructor can't have arguments.");
		if(type->dtor)
			t.Throw("Type '%s' have already declared destructor.", type->name.c_str());
		type->dtor = func.Get();
	}
	if(type)
	{
		if(FindEqualFunction(type, AnyFunction(func)))
			t.Throw("Method '%s' already exists.", GetName(func));
		type->ufuncs.push_back(func);
	}
	else
	{
		if(FindEqualFunction(func))
			t.Throw("Function '%s' already exists.", GetName(func));
	}

#ifdef _DEBUG
	func->decl = GetName(func);
#endif
	ufuncs.push_back(func);
	return func.Pin();
}

VarType Parser::AnalyzeVarType()
{
	VarType result(V_VOID, 0);
	if(t.IsKeywordGroup(G_VAR))
		result.type = t.GetKeywordId(G_VAR);
	else if(t.IsItem())
	{
		Type* type = AnalyzeAddType(t.GetItem());
		result.type = type->index;
	}
	else
		t.Unexpected();
	t.Next();

	if(t.IsSymbol('&'))
	{
		result.subtype = result.type;
		result.type = V_REF;
		t.Next();
	}

	return result;
}

Type* Parser::AnalyzeAddType(const string& name) 
{
	Type* type = new Type;
	type->module = nullptr;
	type->name = name;
	type->index = (0xFFFF0000 | module->tmp_types.size());
	type->declared = false;
	type->first_line = t.GetLine();
	type->first_charpos = t.GetCharPos();
	type->flags = 0;
	module->tmp_types.push_back(type);
	AddType(type);
	return type;
}

void Parser::AnalyzeMakeType(VarType& vartype, const string& name)
{
	if(vartype.type == V_SPECIAL)
	{
		Type* result_type = AnalyzeAddType(name);
		vartype = VarType(result_type->index, 0);
	}
	else if(vartype.subtype == V_SPECIAL)
	{
		Type* result_type = AnalyzeAddType(name);
		vartype = VarType(V_REF, result_type->index);
	}
}

// create default functions for type if they are not declared
// return flags for builtin functions to declare in module
int Parser::CreateDefaultFunctions(Type* type, int define_ctor)
{
	assert(type);
	VarType vartype(type->index, 0);
	int result = 0;

	// ctor
	AnyFunction f = FindSpecialFunction(type, SF_CTOR, [](AnyFunction& f) { return true; });
	bool apply_ctor_code = false;
	if(!f)
	{
		// decl
		ParseFunction* func = new ParseFunction;
		func->name = type->name;
		func->result = vartype;
		func->arg_infos.push_back(ArgInfo(vartype, 0, false));
		func->block = nullptr;
		func->flags = CommonFunction::F_DEFAULT;
		func->index = ufuncs.size();
		func->type = type->index;
		func->required_args = 1;
		func->special = SF_CTOR;
#ifdef _DEBUG
		func->decl = GetName(func);
#endif
		ufuncs.push_back(func);
		type->ufuncs.push_back(func);
		if(define_ctor == -1)
			apply_ctor_code = true;
		f = func;
	}
	else if(define_ctor == 0 && f.pf->IsDefault())
		apply_ctor_code = true;
	if(apply_ctor_code)
	{
		// code
		assert(f.IsParse());
		ParseFunction* func = f.pf;
		ParseNode* group = ParseNode::Get();
		group->result = vartype;
		group->pseudo_op = INTERNAL_GROUP;
		for(Member* m : type->members)
		{
			ParseNode* node = ParseNode::Get();
			SetParseNodeFromMember(node, m);
			group->push(node);
			group->push(SET_THIS_MEMBER, m->index);
			group->push(POP);
		}
		group->push(RET);
		func->node = group;
	}
	if(define_ctor == 0)
		return 0;

	// copy ctor
	if(type->IsStruct())
	{
		f = FindSpecialFunction(type, SF_CTOR, [vartype](AnyFunction& f)
		{
			uint offset = (f.cf->IsCode() ? 0 : 1);
			return f.cf->arg_infos.size() == offset + 1 && In(f.cf->arg_infos[offset].vartype, { vartype, VarType(V_REF, vartype.type) });
		});
		if(!f)
		{
			// decl
			ParseFunction* func = new ParseFunction;
			func->name = type->name;
			func->result = vartype;
			func->arg_infos.push_back(ArgInfo(vartype, 0, false));
			func->arg_infos.push_back(ArgInfo(vartype, 0, false));
			func->block = nullptr;
			func->flags = CommonFunction::F_DEFAULT | CommonFunction::F_BUILTIN;
			func->index = ufuncs.size();
			func->type = type->index;
			func->required_args = 2;
			func->special = SF_CTOR;
			func->node = nullptr;
#ifdef _DEBUG
			func->decl = GetName(func);
#endif
			ufuncs.push_back(func);
			type->ufuncs.push_back(func);
		}
	}

	// assign
	SymbolInfo* info = &symbols[S_ASSIGN];
	f = FindFunction(type, info->op_code, [type](AnyFunction& f) { return f.cf->arg_infos[1].vartype.type == type->index; });
	if(!f)
	{
		if(IS_ALL_SET(type->flags, Type::Code | Type::Ref))
			result |= BF_ASSIGN;
		else
		{
			ParseFunction* func = new ParseFunction;
			func->name = info->op_code;
			func->result = vartype;
			func->index = ufuncs.size();
			func->type = type->index;
			func->arg_infos.push_back(ArgInfo(vartype, 0, false));
			func->arg_infos.push_back(ArgInfo(vartype, 0, false));
			func->required_args = 2;
			func->special = SF_NO;
			func->flags = CommonFunction::F_DEFAULT;
			if(type->IsStruct())
			{
				ParseNode* group = ParseNode::Get();
				for(Member* m : type->members)
				{
					ParseNode* set = ParseNode::Get();
					set->op = SET_THIS_MEMBER;
					set->result = m->vartype;
					set->value = m->index;
					set->source = nullptr;
					set->push(PUSH_ARG, 1);
					set->push(PUSH_MEMBER, m->index);
					group->push(set);
				}
				ParseNode* ret = ParseNode::Get();
				ret->pseudo_op = RETURN;
				ret->source = nullptr;
				ret->result = V_VOID;
				ret->push(PUSH_THIS);
				group->push(ret);
				group->pseudo_op = GROUP;
				func->node = group;
			}
			else
			{
				func->flags |= CommonFunction::F_BUILTIN;
				func->node = nullptr;
			}
			func->block = nullptr;
#ifdef _DEBUG
			func->decl = GetName(func);
#endif
			ufuncs.push_back(func);
			type->ufuncs.push_back(func);
		}
	}

	// equal
	info = &symbols[S_EQUAL];
	f = FindFunction(type, info->op_code, [type](AnyFunction& f) { return f.cf->arg_infos[1].vartype.type == type->index; });
	if(!f)
	{
		if(IS_ALL_SET(type->flags, Type::Code | Type::Ref))
			result |= BF_EQUAL;
		else
		{
			ParseFunction* func = new ParseFunction;
			func->name = info->op_code;
			func->result = V_BOOL;
			func->index = ufuncs.size();
			func->type = type->index;
			func->arg_infos.push_back(ArgInfo(vartype, 0, false));
			func->arg_infos.push_back(ArgInfo(vartype, 0, false));
			func->required_args = 2;
			func->special = SF_NO;
			func->flags = CommonFunction::F_DEFAULT;
			if(type->IsStruct())
			{
				ParseNode* group = ParseNode::Get();
				bool first = true;
				for(Member* m : type->members)
				{
					group->push(PUSH_THIS_MEMBER, m->index);
					group->push(PUSH_ARG, 1);
					group->push(PUSH_MEMBER, m->index);
					group->push(EQ);
					if(!first)
						group->push(AND);
					else
						first = false;
				}
				group->push(RET);
				group->result = V_BOOL;
				group->pseudo_op = INTERNAL_GROUP;
				func->node = group;
			}
			else
			{
				func->flags |= CommonFunction::F_BUILTIN;
				func->node = nullptr;
			}
			func->block = nullptr;
#ifdef _DEBUG
			func->decl = GetName(func);
#endif
			ufuncs.push_back(func);
			type->ufuncs.push_back(func);
		}
	}

	// not equal
	info = &symbols[S_NOT_EQUAL];
	f = FindFunction(type, info->op_code, [type](AnyFunction& f) { return f.cf->arg_infos[1].vartype.type == type->index; });
	if(!f)
	{
		if(IS_ALL_SET(type->flags, Type::Code | Type::Ref))
			result |= BF_NOT_EQUAL;
		else
		{
			ParseFunction* func = new ParseFunction;
			func->name = info->op_code;
			func->result = V_BOOL;
			func->index = ufuncs.size();
			func->type = type->index;
			func->arg_infos.push_back(ArgInfo(vartype, 0, false));
			func->arg_infos.push_back(ArgInfo(vartype, 0, false));
			func->required_args = 2;
			func->special = SF_NO;
			func->flags = CommonFunction::F_DEFAULT;
			if(type->IsStruct())
			{
				ParseNode* group = ParseNode::Get();
				bool first = true;
				for(Member* m : type->members)
				{
					group->push(PUSH_THIS_MEMBER, m->index);
					group->push(PUSH_ARG, 1);
					group->push(PUSH_MEMBER, m->index);
					group->push(NOT_EQ);
					if(!first)
						group->push(OR);
					else
						first = false;
				}
				group->push(RET);
				group->result = V_BOOL;
				group->pseudo_op = INTERNAL_GROUP;
				func->node = group;
			}
			else
			{
				func->flags |= CommonFunction::F_BUILTIN;
				func->node = nullptr;
			}
			func->block = nullptr;
#ifdef _DEBUG
			func->decl = GetName(func);
#endif
			ufuncs.push_back(func);
			type->ufuncs.push_back(func);
		}
	}

	return result;
}

bool Parser::CheckTypeLoop(Type* type)
{
	return false;
}

void Parser::SetParseNodeFromMember(ParseNode* node, Member* m)
{
	if(m->have_def_value)
	{
		switch(m->vartype.type)
		{
		case V_BOOL:
			node->pseudo_op = PUSH_BOOL;
			node->bvalue = m->bvalue;
			break;
		case V_CHAR:
			node->op = PUSH_CHAR;
			node->cvalue = m->cvalue;
			break;
		case V_INT:
			node->op = PUSH_INT;
			node->value = m->value;
			break;
		case V_FLOAT:
			node->op = PUSH_FLOAT;
			node->fvalue = m->fvalue;
			break;
		case V_STRING:
			node->op = PUSH_STRING;
			node->value = m->value;
			break;
		default:
			// will write error in parsing phase
			break;
		}
	}
	else
	{
		switch(m->vartype.type)
		{
		case V_BOOL:
			node->pseudo_op = PUSH_BOOL;
			node->bvalue = false;
			break;
		case V_CHAR:
			node->op = PUSH_CHAR;
			node->cvalue = 0;
			break;
		case V_INT:
			node->op = PUSH_INT;
			node->value = 0;
			break;
		case V_FLOAT:
			node->op = PUSH_FLOAT;
			node->fvalue = 0;
			break;
		case V_STRING:
			node->op = PUSH_STRING;
			node->value = GetEmptyString();
			break;
		default:
			// will write error in parsing phase
			break;
		}
	}
	node->result = m->vartype;
}

bool Parser::HasSideEffects(ParseNode* node)
{
	switch(node->op)
	{
	case PUSH:
	case PUSH_TRUE:
	case PUSH_FALSE:
	case PUSH_TMP:
		// not allowed in ParseNode, only part of GROUP/INTERNAL_GROUP
		assert(0);
		return true;
	case PUSH_CHAR:
	case PUSH_INT:
	case PUSH_FLOAT:
	case PUSH_STRING:
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
	case PUSH_INDEX:
	case PUSH_BOOL:
	case PUSH_TYPE:
		return false;
	default:
		return true;
	}
}

void Parser::VerifyAllTypes()
{
	// verify all types are declared, member default values
	for(Type* type : module->tmp_types)
	{
		if(!type->declared)
			t.ThrowAt(type->first_line, type->first_charpos, "Undeclared type '%s' used.", type->name.c_str());
		if(type->have_def_value)
		{
			for(Member* m : type->members)
			{
				if(!m->have_def_value)
					continue;
				t.MoveTo(m->pos);
				if(t.IsSymbol('='))
				{
					t.Next();
					NodeRef item = ParseConstExpr();
					if(!TryCast(item.Get(), m->vartype))
						t.Throw("Can't assign type '%s' to member '%s.%s'.", GetTypeName(item), type->name.c_str(), m->name.c_str());
					m->value = item->value;
				}
				else
				{
					assert(t.IsSymbol('('));
					ParseNode* node = ParseVarCtor(m->vartype);
					assert(node->op != CALLU_CTOR && node->op != CALL);
					m->value = node->value;
					node->Free();
				}
			}
			CreateDefaultFunctions(type, 0);
		}
	}
}

void Parser::VerifyFunctionsDefaultArguments()
{
	// function default values
	for(ParseFunction* f : ufuncs)
	{
		if(f->required_args == f->arg_infos.size() || f->IsDefault())
			continue;
		AnalyzeArgsDefaultValues(f);
	}
}

void Parser::AnalyzeArgsDefaultValues(ParseFunction* f)
{
	t.MoveTo(f->start_pos);
	uint offset = (f->special == SF_CTOR ? 1u : 0u);

	for(uint i = offset, count = f->arg_infos.size(); i <= count; ++i)
	{
		VarType vartype = AnalyzeVarType();
		t.AssertItem();
		t.Next();

		if(t.IsSymbol('='))
		{
			t.Next();
			NodeRef item = ParseConstExpr();
			if(!TryConstCast(item.Get(), vartype))
				t.Throw("Invalid default value of type '%s', required '%s'.", GetTypeName(item), GetName(vartype));
			ArgInfo& arg = f->arg_infos[i];
			switch(item->op)
			{
			case PUSH_BOOL:
				arg.bvalue = item->bvalue;
				break;
			case PUSH_CHAR:
				arg.cvalue = item->cvalue;
				break;
			case PUSH_INT:
			case PUSH_STRING:
			case PUSH_ENUM:
				arg.value = item->value;
				break;
			case PUSH_FLOAT:
				arg.fvalue = item->fvalue;
				break;
			default:
				assert(0);
				break;
			}
		}
		if(t.IsSymbol(')'))
			break;
		t.AssertSymbol(',');
		t.Next();
	}
}
