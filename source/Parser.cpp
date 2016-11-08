#include "Pch.h"
#include "CasImpl.h"
#include "Parser.h"
#include "ParserImpl.h"
#include "Module.h"

// http://en.cppreference.com/w/cpp/language/operator_precedence
SymbolInfo symbols[] = {
	S_ADD, "add", 6, true, 2, ADD, ST_NONE, "$opAdd",
	S_SUB, "subtract", 6, true, 2, SUB, ST_NONE, "$opSub",
	S_MUL, "multiply", 5, true, 2, MUL, ST_NONE, "$opMul",
	S_DIV, "divide", 5, true, 2, DIV, ST_NONE, "$opDiv",
	S_MOD, "modulo", 5, true, 2, MOD, ST_NONE, "$opMod",
	S_BIT_AND, "bit and", 10, true, 2, BIT_AND, ST_NONE, "$opBitAnd",
	S_BIT_OR, "bit or", 12, true, 2, BIT_OR, ST_NONE, "$opBitOr",
	S_BIT_XOR, "bit xor", 11, true, 2, BIT_XOR, ST_NONE, "$opBitXor",
	S_BIT_LSHIFT, "bit left shift", 7, true, 2, BIT_LSHIFT, ST_NONE, "$opBitLshift",
	S_BIT_RSHIFT, "bit right shift", 7, true, 2, BIT_RSHIFT, ST_NONE, "$opBitRshift",
	S_PLUS, "unary plus", 3, false, 1, NOP, ST_NONE, "$opPlus",
	S_MINUS, "unary minus", 3, false, 1, NEG, ST_NONE, "$opMinus",
	S_EQUAL, "equal", 9, true, 2, EQ, ST_NONE, "$opEqual",
	S_NOT_EQUAL, "not equal", 9, true, 2, NOT_EQ, ST_NONE, "$opNotEqual",
	S_GREATER, "greater", 8, true, 2, GR, ST_NONE, "$opGreater",
	S_GREATER_EQUAL, "greater equal", 8, true, 2, GR_EQ, ST_NONE, "$opGreaterEqual",
	S_LESS, "less", 8, true, 2, LE, ST_NONE, "$opLess",
	S_LESS_EQUAL, "less equal", 8, true, 2, LE_EQ, ST_NONE, "$opLessEqual",
	S_AND, "and", 13, true, 2, AND, ST_NONE, nullptr,
	S_OR, "or", 14, true, 2, OR, ST_NONE, nullptr,
	S_NOT, "not", 3, false, 1, NOT, ST_NONE,  "$opNot",
	S_BIT_NOT, "bit not", 3, false, 1, BIT_NOT, ST_NONE, "$opBitNot",
	S_MEMBER_ACCESS, "member access", 2, true, 2, NOP, ST_NONE, nullptr,
	S_ASSIGN, "assign", 15, false, 2, NOP, ST_ASSIGN, nullptr,
	S_ASSIGN_ADD, "assign add", 15, false, 2, S_ADD, ST_ASSIGN, "$opAssignAdd",
	S_ASSIGN_SUB, "assign subtract", 15, false, 2, S_SUB, ST_ASSIGN, "$opAssignSub",
	S_ASSIGN_MUL, "assign multiply", 15, false, 2, S_MUL, ST_ASSIGN, "$opAssignMul",
	S_ASSIGN_DIV, "assign divide", 15, false, 2, S_DIV, ST_ASSIGN, "$opAssignDiv",
	S_ASSIGN_MOD, "assign modulo", 15, false, 2, S_MOD, ST_ASSIGN, "$opAssignMod",
	S_ASSIGN_BIT_AND, "assign bit and", 15, false, 2, S_BIT_AND, ST_ASSIGN, "$opAssignBitAnd",
	S_ASSIGN_BIT_OR, "assign bit or", 15, false, 2, S_BIT_OR, ST_ASSIGN, "$opAssignBitOr",
	S_ASSIGN_BIT_XOR, "assign bit xor", 15, false, 2, S_BIT_XOR, ST_ASSIGN, "$opAssignBitXor",
	S_ASSIGN_BIT_LSHIFT, "assign bit left shift", 15, false, 2, S_BIT_LSHIFT, ST_ASSIGN, "$opAssignBitLshift",
	S_ASSIGN_BIT_RSHIFT, "assign bit right shift", 15, false, 2, S_BIT_RSHIFT, ST_ASSIGN, "$opAssignBitRshift",
	S_PRE_INC, "pre increment", 3, false, 1, INC, ST_INC_DEC, "$opPreInc",
	S_PRE_DEC, "pre decrement", 3, false, 1, DEC, ST_INC_DEC, "$opPreDec",
	S_POST_INC, "post increment", 2, true, 1, INC, ST_INC_DEC, "$opPostInc",
	S_POST_DEC, "post decrement", 2, true, 1, DEC, ST_INC_DEC, "$opPostDec",
	S_IS, "reference equal", 9, true, 2, IS, ST_NONE, nullptr,
	S_SUBSCRIPT, "subscript", 2, true, 1, NOP, ST_SUBSCRIPT, "$opIndex",
	S_INVALID, "invalid", 99, true, 0, NOP, ST_NONE, nullptr
};
static_assert(sizeof(symbols) / sizeof(SymbolInfo) == S_MAX, "Missing symbols.");

BasicSymbolInfo basic_symbols[] = {
	BS_INC, "+", S_PLUS, S_INVALID, S_ADD,
	BS_DEC, "-", S_MINUS, S_INVALID, S_SUB,
	BS_MUL, "*", S_INVALID, S_INVALID, S_MUL,
	BS_DIV, "/", S_INVALID, S_INVALID, S_DIV,
	BS_MOD, "%", S_INVALID, S_INVALID, S_MOD,
	BS_BIT_AND, "&", S_INVALID, S_INVALID, S_BIT_AND,
	BS_BIT_OR, "|", S_INVALID, S_INVALID, S_BIT_OR,
	BS_BIT_XOR, "^", S_INVALID, S_INVALID, S_BIT_XOR,
	BS_BIT_LSHIFT, "<<", S_INVALID, S_INVALID, S_BIT_LSHIFT,
	BS_BIT_RSHIFT, ">>", S_INVALID, S_INVALID, S_BIT_RSHIFT,
	BS_EQUAL, "==", S_INVALID, S_INVALID, S_EQUAL,
	BS_NOT_EQUAL, "!=", S_INVALID, S_INVALID, S_NOT_EQUAL,
	BS_GREATER, ">", S_INVALID, S_INVALID, S_GREATER,
	BS_GREATER_EQUAL, ">=", S_INVALID, S_INVALID, S_GREATER_EQUAL,
	BS_LESS, "<", S_INVALID, S_INVALID, S_LESS,
	BS_LESS_EQUAL, "<=", S_INVALID, S_INVALID, S_LESS_EQUAL,
	BS_AND, "&&", S_INVALID, S_INVALID, S_AND,
	BS_OR, "||", S_INVALID, S_INVALID, S_OR,
	BS_NOT, "!", S_NOT, S_INVALID, S_INVALID,
	BS_BIT_NOT, "~", S_BIT_NOT, S_INVALID, S_INVALID,
	BS_ASSIGN, "=", S_INVALID, S_INVALID, S_ASSIGN,
	BS_ASSIGN_ADD, "+=", S_INVALID, S_INVALID, S_ASSIGN_ADD,
	BS_ASSIGN_SUB, "-=", S_INVALID, S_INVALID, S_ASSIGN_SUB,
	BS_ASSIGN_MUL, "*=", S_INVALID, S_INVALID, S_ASSIGN_MUL,
	BS_ASSIGN_DIV, "/=", S_INVALID, S_INVALID, S_ASSIGN_DIV,
	BS_ASSIGN_MOD, "%=", S_INVALID, S_INVALID, S_ASSIGN_MOD,
	BS_ASSIGN_BIT_AND, "&=", S_INVALID, S_INVALID, S_ASSIGN_BIT_AND,
	BS_ASSIGN_BIT_OR, "|=", S_INVALID, S_INVALID, S_ASSIGN_BIT_OR,
	BS_ASSIGN_BIT_XOR, "^=", S_INVALID, S_INVALID, S_ASSIGN_BIT_XOR,
	BS_ASSIGN_BIT_LSHIFT, "<<=", S_INVALID, S_INVALID, S_ASSIGN_BIT_LSHIFT,
	BS_ASSIGN_BIT_RSHIFT, ">>=", S_INVALID, S_INVALID, S_ASSIGN_BIT_RSHIFT,
	BS_DOT, ".", S_INVALID, S_INVALID, S_MEMBER_ACCESS,
	BS_INC, "++", S_PRE_INC, S_POST_INC, S_INVALID,
	BS_DEC, "--", S_PRE_DEC, S_POST_DEC, S_INVALID,
	BS_IS, "is", S_INVALID, S_INVALID, S_IS,
	BS_SUBSCRIPT, "[", S_INVALID, S_SUBSCRIPT, S_INVALID
};
static_assert(sizeof(basic_symbols) / sizeof(BasicSymbolInfo) == BS_MAX, "Missing basic symbols.");

Parser::Parser(Module* module) : module(module), t(Tokenizer::F_SEEK | Tokenizer::F_UNESCAPE | Tokenizer::F_CHAR), run_module(nullptr)
{
	AddKeywords();
	AddChildModulesKeywords();
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
		{ "operator", K_OPERATOR }
	});

	// const
	t.AddKeywords(G_CONST, {
		{ "true", C_TRUE },
		{ "false", C_FALSE }
	});
}

void Parser::AddChildModulesKeywords()
{
	for(auto& m : module->modules)
	{
		if(m.first == module->index)
			continue;
		for(Type* type : m.second->types)
		{
			if(!IS_SET(type->flags, Type::Hidden))
				AddType(type);
		}
	}
}

RunModule* Parser::Parse(ParseSettings& settings)
{
	optimize = settings.optimize;
	t.FromString(settings.input);
	run_module = new RunModule(module);

	try
	{
		ParseCode();
		Optimize();
		CheckReturnValues();
		CopyFunctionChangedStructs();
		ConvertToBytecode();
		FinishRunModule();
	}
	catch(const Tokenizer::Exception& e)
	{
		Event(EventType::Error, e.ToString());
		Cleanup();
	}
	
	return run_module;
}

void Parser::FinishRunModule()
{
	run_module->strs = strs;
	run_module->ufuncs.resize(ufuncs.size());
	for(uint i = 0; i < ufuncs.size(); ++i)
	{
		ParseFunction& f = *ufuncs[i];
		UserFunction& uf = run_module->ufuncs[i];
		uf.pos = f.pos;
		uf.locals = f.locals;
		uf.result = f.result;
		for(auto& arg : f.arg_infos)
			uf.args.push_back(arg.type);
		uf.type = f.type;
	}
	run_module->globals = main_block->GetMaxVars();
	run_module->result = global_result;
}

void Parser::Cleanup()
{
	assert(run_module);

	// cleanup old types
	for(Type* type : run_module->types)
	{
		t.RemoveKeyword(type->name.c_str(), type->index, G_VAR);
		delete type;
	}

	Str::Free(strs);
	DeleteElements(ufuncs);
	DeleteElements(rsvs);
	delete run_module;
	main_block->Free();
	if(global_node)
	{
		global_node->Free();
		global_node = nullptr;
	}
	run_module = nullptr;
	main_block = nullptr;
}

void Parser::ParseCode()
{
	breakable_block = 0;
	empty_string = -1;
	main_block = Block::Get();
	main_block->parent = nullptr;
	main_block->var_offset = 0u;
	current_block = main_block;
	current_function = nullptr;
	current_type = nullptr;
	global_node = nullptr;
	global_returns.clear();

	NodeRef node;
	node->pseudo_op = GROUP;
	node->type = V_VOID;
	node->ref = REF_NO;

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
		return ParseLine();
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

	current_block = old_block;

	if(group->childs.empty())
		return nullptr;
	else
	{
		group->pseudo_op = GROUP;
		group->type = V_VOID;
		group->ref = REF_NO;
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
				if_op->type = V_VOID;
				if_op->ref = REF_NO;
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
				t.AssertSymbol(';');
				t.Next();
				ParseNode* do_whil = ParseNode::Get();
				do_whil->pseudo_op = DO_WHILE;
				do_whil->type = V_VOID;
				do_whil->value = DO_WHILE_NORMAL;
				do_whil->ref = REF_NO;
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
				whil->type = V_VOID;
				whil->ref = REF_NO;
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

				if(for2 && !TryCast(for2.Get(), VarType(V_BOOL)))
					t.Throw("Condition expression with '%s' type.", GetTypeName(for2));

				NodeRef fo = ParseNode::Get();
				fo->pseudo_op = FOR;
				fo->type = V_VOID;
				fo->ref = REF_NO;
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
				br->type = V_VOID;
				br->ref = REF_NO;
				br->source = nullptr;
				return br;
			}
		case K_RETURN:
			{
				NodeRef ret = ParseNode::Get();
				ret->pseudo_op = RETURN;
				ret->type = V_VOID;
				ret->ref = REF_NO;
				ret->source = nullptr;
				t.Next();
				if(!t.IsSymbol(';'))
				{
					ret->push(ParseExpr(';'));
					t.AssertSymbol(';');
				}
				if(current_function)
				{
					VarType req_type;
					if(current_function->special == SF_CTOR)
						req_type = VarType(V_VOID);
					else
						req_type = current_function->result;

					bool ok = true;
					cstring type_name = nullptr;
					if(ret->childs.empty())
					{
						if(req_type.core != V_VOID)
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
							type_name = GetName(r->GetVarType());
						}

						if(ok && current_function->special == SF_NO && current_function->result.special == SV_REF)
						{
							if(r->op == PUSH_LOCAL_REF || r->op == PUSH_ARG_REF)
								t.Throw("Returning reference to temporary variable '%s'.", GetName(GetVar(r)));
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
						if((r->type != V_INT && r->type != V_BOOL && r->type != V_FLOAT) || r->ref == REF_YES)
							t.Throw("Invalid type '%s' for global return.", GetName(r->GetVarType()));
					}
					ReturnInfo info = { ret, t.GetLine(), t.GetCharPos() };
					global_returns.push_back(info);
				}
				t.Next();
				return ret.Pin();
			}
		case K_CLASS:
		case K_STRUCT:
			{
				if(current_block != main_block)
					t.Throw("%s can't be declared inside block.", keyword == K_CLASS ? "Class" : "Struct");

				// id
				t.Next();
				const string& id = t.MustGetItem();
				CheckFindItem(id, false);
				Type* type = new Type;
				type->name = id;
				type->flags = Type::Class;
				if(keyword == K_CLASS)
					type->flags |= Type::Ref;
				type->index = (0xFFFF0000 | run_module->types.size());
				type->size = 0;
				run_module->types.push_back(type);
				AddType(type);
				t.Next();

				// {
				t.AssertSymbol('{');
				t.Next();

				uint pad = 0;
				current_type = type;

				// [class_item ...] }
				while(!t.IsSymbol('}'))
				{
					// member, method or ctor
					int var_type = t.MustGetKeywordId(G_VAR);
					bool is_func;
					t.SeekStart();
					if(t.SeekSymbol('&'))
						t.SeekNext();
					if(t.SeekSymbol('('))
						is_func = true; // ctor
					else if(t.SeekItem())
					{
						t.SeekNext();
						if(t.SeekSymbol('('))
							is_func = true; // method
						else
							is_func = false; // member
					}
					else
						is_func = false; // invalid, will fail at parsing below

					if(is_func)
					{
						ParseFunction* f = new ParseFunction;
						f->result = VarType(var_type);
						t.Next();
						if(t.IsSymbol('&'))
						{
							f->result.special = SV_REF;
							t.Next();
						}

						if(VarType(type->index) == f->result && t.IsSymbol('('))
						{
							// ctor
							if(f->result.special == SV_REF)
								t.Unexpected();
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

						f->type = type->index;
						ParseFunctionArgs(f, false);
						f->arg_infos.insert(f->arg_infos.begin(), ArgInfo(VarType(type->index), 0, false));
						f->required_args++;

						if(FindEqualFunction(f))
							t.Throw("Function '%s' already exists.", GetName(f));

						// block
						f->node = ParseBlock(f);
						current_function = nullptr;
						if(f->special == SF_CTOR)
							type->flags |= Type::HaveCtor;
						f->index = ufuncs.size();
						ufuncs.push_back(f);
						type->ufuncs.push_back(f);
					}
					else
						ParseMemberDeclClass(type, pad);
				}
				t.Next();

				current_type = nullptr;
				return nullptr;
			}
		default:
			t.Unexpected();
		}
	}
	else if(t.IsKeywordGroup(G_VAR))
	{
		// is this function or var declaration or ctor
		VarType type = GetVarType(false);

		if(t.IsSymbol('('))
		{
			// ctor
			int var_type = type.core;
			NodeRef node = ParseExpr(';', 0, &var_type);
			t.AssertSymbol(';');
			t.Next();
			return node.Pin();
		}

		LocalString str = t.MustGetItem();
		CheckFindItem(str.get_ref(), true);
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
			f->type = V_VOID;
			f->special = SF_NO;
			f->node = nullptr;
			f->block = nullptr;

			// args
			ParseFunctionArgs(f, false);
			if(FindEqualFunction(f))
			{
				cstring name = GetName(f);
				delete f;
				t.Throw("Function '%s' already exists.", name);
			}

			// block
			f->node = ParseBlock(f);
			current_function = nullptr;
			ufuncs.push_back(f);
			return nullptr;
		}
		else
		{
			// var
			if(type.special == SV_REF)
				t.Throw("Reference variable unavailable yet.");
			ParseNode* node = ParseVarTypeDecl(&type, str.get_ptr());
			t.Next();
			return node;
		}
	}

	NodeRef node = ParseExpr(';');

	// ;
	t.AssertSymbol(';');
	t.Next();

	return node.Pin();
}

// func_decl
Function* Parser::ParseFuncDecl(cstring decl, Type* type)
{
	assert(decl);

	Function* f = new Function;

	try
	{
		t.FromString(decl);
		t.Next();

		f->result = GetVarType(true);

		/*if(t.IsKeyword(K_OPERATOR, G_KEYWORD))
		{
			t.Next();
			f->name = t.MustGetItem();
			if(f->name == "addref")
				f->special = SF_ADDREF;
			else if(f->name == "release")
				f->special = SF_RELEASE;
			else
				t.Throw("Invalid operator name.");
			t.Next();
		}*/

		if(type && type->index == f->result.core && t.IsSymbol('('))
		{
			// ctor
			f->special = SF_CTOR;
			f->name = type->name;
		}
		else
		{
			if(t.IsKeyword(K_OPERATOR, G_KEYWORD))
			{
				t.Next();
			}
			else
			{
				f->special = SF_NO;
				f->name = t.MustGetItem();
				t.Next();
			}
		}

		t.AssertSymbol('(');
		t.Next();

		ParseFunctionArgs(f, true);

		t.AssertEof();

		// verify args, return type
		/*switch(f->special)
		{
		case SF_NO:
		case SF_CTOR:
			break;
		case SF_ADDREF:
		case SF_RELEASE:
			if(f->result.core != V_VOID || !f->arg_infos.empty())
				t.Throw("Special function '%s' require declaration of 'void %s()' instead of '%s'.", f->name.c_str(), f->name.c_str(), GetName(f));
			break;
		default:
			assert(0);
			break;
		}*/
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

		m->type = GetVarTypeForMember();

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
	int var_type = GetVarTypeForMember();
	uint var_size = GetType(var_type)->size;
	assert(var_size == 1 || var_size == 4);

	do
	{
		Member* m = new Member;
		m->type = var_type;
		m->name = t.MustGetItem();
		int index;
		if(type->FindMember(m->name, index))
			t.Throw("Member with name '%s.%s' already exists.", type->name.c_str(), m->name.c_str());
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
		type->members.push_back(m);

		if(t.IsSymbol(';'))
			break;
		t.AssertSymbol(',');
		t.Next();
	} while(1);

	t.Next();
}

void Parser::ParseFuncInfo(CommonFunction* f, Type* type, bool in_cpp)
{
	BASIC_SYMBOL symbol = BS_MAX;
	f->result = GetVarType(in_cpp);
	
	if(type && type->index == f->result.core && t.IsSymbol('('))
	{
		// ctor
		f->special = SF_CTOR;
		f->name = type->name;
	}
	else
	{
		if(t.IsKeyword(K_OPERATOR, G_KEYWORD))
		{
			t.Next();
			symbol = GetSymbol();
			if(!CanOverload(symbol))
				t.Throw("Can't overload operator '%s'.", basic_symbols[symbol].text);
			t.Next();
		}
		else
		{
			f->special = SF_NO;
			f->name = t.MustGetItem();
			t.Next();
		}
	}

	t.AssertSymbol('(');
	t.Next();

	ParseFunctionArgs(f, true);

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
	if(!in_cpp)
		current_function = (ParseFunction*)f;

	// args
	bool prev_arg_def = false;
	if(!t.IsSymbol(')'))
	{
		while(true)
		{
			VarType type = GetVarType(in_cpp);
			LocalString id = t.MustGetItem();
			CheckFindItem(id.get_ref(), false);
			if(!in_cpp)
			{
				ParseVar* arg = ParseVar::Get();
				arg->name = id;
				arg->type = type;
				arg->subtype = ParseVar::ARG;
				arg->index = current_function->args.size();
				arg->mod = false;
				current_function->args.push_back(arg);
			}
			t.Next();
			if(t.IsSymbol('='))
			{
				prev_arg_def = true;
				t.Next();
				ParseNode* item = ParseConstItem();
				if(!TryCast(item, type))
					t.Throw("Invalid default value of type '%s', required '%s'.", GetTypeName(item), GetName(type));
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
				default:
					assert(0);
					break;
				}
			}
			else
			{
				if(prev_arg_def)
					t.Throw("Missing default value for argument '%s'.", id.c_str());
				f->arg_infos.push_back(ArgInfo(type, 0, false));
				f->required_args++;
			}
			if(t.IsSymbol(')'))
				break;
			t.AssertSymbol(',');
			t.Next();
		}
	}
	t.Next();
}

ParseNode* Parser::ParseVarTypeDecl(VarType* _type, string* _name)
{
	// var_type
	VarType type;
	if(_type)
		type = *_type;
	else
		type = GetVarType(false);
	if(type.core == V_VOID)
		t.Throw("Can't declare void variable.");
	if(type.special == SV_REF)
		t.Throw("Can't declare reference variable yet.");

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
	else
	{
		ParseNode* node = ParseNode::Get();
		node->pseudo_op = GROUP;
		node->type = V_VOID;
		node->ref = REF_NO;
		node->source = nullptr;
		node->childs = nodes;
		return node;
	}
}

ParseNode* Parser::ParseCond()
{
	t.AssertSymbol('(');
	t.Next();
	NodeRef cond = ParseExpr(')');
	t.AssertSymbol(')');
	if(!TryCast(cond.Get(), VarType(V_BOOL)))
		t.Throw("Condition expression with '%s' type.", GetTypeName(cond));
	t.Next();
	return cond.Pin();
}

ParseNode* Parser::ParseVarDecl(VarType type, string* _name)
{
	// var_name
	const string& name = (_name ? *_name : t.MustGetItem());
	if(!_name)
		CheckFindItem(name, false);

	ParseVar* var = ParseVar::Get();
	var->name = name;
	var->type = type;
	var->index = current_block->var_offset;
	var->subtype = (current_function == nullptr ? ParseVar::GLOBAL : ParseVar::LOCAL);
	var->mod = false;
	current_block->vars.push_back(var);
	current_block->var_offset++;
	if(!_name)
		t.Next();

	// [=]
	NodeRef expr(nullptr);
	if(!t.IsSymbol('='))
	{
		expr = ParseNode::Get();
		expr->ref = REF_NO;
		expr->source = nullptr;
		switch(type.core)
		{
		case V_BOOL:
			expr->type = V_BOOL;
			expr->pseudo_op = PUSH_BOOL;
			expr->bvalue = false;
			break;
		case V_CHAR:
			expr->type = V_CHAR;
			expr->op = PUSH_CHAR;
			expr->cvalue = 0;
			break;
		case V_INT:
			expr->type = V_INT;
			expr->op = PUSH_INT;
			expr->value = 0;
			break;
		case V_FLOAT:
			expr->type = V_FLOAT;
			expr->op = PUSH_FLOAT;
			expr->fvalue = 0.f;
			break;
		case V_STRING:
			expr->type = V_STRING;
			expr->op = PUSH_STRING;
			if(empty_string == -1)
			{
				empty_string = strs.size();
				Str* str = Str::Get();
				str->s = "";
				str->refs = 1;
				strs.push_back(str);
			}
			expr->value = empty_string;
			break;
		default: // class
			{
				Type* rtype = GetType(type.core);
				if(IS_SET(rtype->flags, Type::HaveCtor))
				{
					vector<AnyFunction> funcs;
					FindAllCtors(rtype, funcs);
					ApplyFunctionCall(expr, funcs, rtype, true);
				}
				else
				{
					expr->type = type.core;
					expr->op = CTOR;
					expr->value = type.core;
				}
			}
			break;
		}
	}
	else
	{
		t.Next();

		// expr<,;>
		expr = ParseExpr(',', ';');
		if(!TryCast(expr.Get(), type))
			t.Throw("Can't assign type '%s' to variable '%s'.", GetTypeName(expr), GetName(var));
	}

	ParseNode* node = ParseNode::Get();
	node->op = (var->subtype == ParseVar::GLOBAL ? SET_GLOBAL : SET_LOCAL);
	node->type = var->type.core;
	node->value = var->index;
	node->ref = REF_NO;
	node->source = nullptr;
	node->push(expr.Pin());
	return node;
}

ParseNode* Parser::ParseExpr(char end, char end2, int* type)
{
	vector<SymbolNode> stack, exit;
	vector<ParseNode*> stack2;

	try
	{
		ParseExprConvertToRPN(exit, stack, type);

		for(SymbolNode& sn : exit)
		{
			if(sn.is_symbol)
				ParseExprApplySymbol(stack2, sn);
			else
			{
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

void Parser::ParseExprConvertToRPN(vector<SymbolNode>& exit, vector<SymbolNode>& stack, int* type)
{
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
				NodeRef node = ParseNode::Get();
				node->type = V_VOID;
				node->str = str;
				node->ref = REF_NO;
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

				// post
				if(GetNextSymbol(left))
				{
					BasicSymbolInfo& bsi = basic_symbols[left];
					if(bsi.post_symbol != S_INVALID)
					{
						PushSymbol(bsi.post_symbol, exit, stack);
						t.Next();
						left = BS_MAX;
					}
				}

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

BASIC_SYMBOL Parser::ParseExprPart(vector<SymbolNode>& exit, vector<SymbolNode>& stack, int* type)
{
	BASIC_SYMBOL symbol = BS_MAX;

	if(!type)
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

	// [post_op ...]
	while(GetNextSymbol(symbol))
	{
		BasicSymbolInfo& bsi = basic_symbols[symbol];
		if(bsi.post_symbol != S_INVALID)
		{
			if(bsi.post_symbol == S_SUBSCRIPT)
			{
				t.Next();
				NodeRef expr = ParseExpr(']');
				if(!TryCast(expr.Get(), VarType(V_INT)))
					t.Throw("Subscript operator require type 'int', found '%s'.", GetTypeName(expr));
				ParseNode* sub = ParseNode::Get();
				sub->pseudo_op = SUBSCRIPT;
				sub->childs.push_back(expr.Pin());
				sub->source = nullptr;
				PushSymbol(bsi.post_symbol, exit, stack, sub);
			}
			else
				PushSymbol(bsi.post_symbol, exit, stack);
			t.Next();
			symbol = BS_MAX;
		}
		else
			break;
	}

	return symbol;
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
			VarType cast;
			int result;
			if(!CanOp(sn.symbol, node->GetVarType(), VarType(V_VOID), cast, result))
				t.Throw("Invalid type '%s' for operation '%s'.", GetTypeName(node), si.name);
			Cast(node.Get(), cast);
			if(!TryConstExpr1(node, si.symbol) && si.op != NOP)
			{
				ParseNode* op = ParseNode::Get();
				op->op = (Op)si.op;
				op->type = result;
				op->push(node.Pin());
				op->ref = REF_NO;
				op->source = nullptr;
				node = op;
			}
			stack.push_back(node.Pin());
		}
		else if(si.type == ST_SUBSCRIPT)
		{
			// subscript operator
			if(node->type != V_STRING)
				t.Throw("Type '%s' don't have subscript operator.", GetTypeName(node));
			ParseNode* op = ParseNode::Get();
			op->op = PUSH_INDEX;
			op->type = V_CHAR;
			op->ref = REF_NO;
			op->source = nullptr;
			op->push(node.Pin());
			op->push(sn.node->childs[0]);
			stack.push_back(op);
			sn.node->childs.clear();
			sn.node->Free();
			sn.node = nullptr;
		}
		else
		{
			// inc dec
			assert(si.type == ST_INC_DEC);
			if(node->type != V_INT && node->type != V_FLOAT && node->type != V_CHAR)
				t.Throw("Invalid type '%s' for operation '%s'.", GetTypeName(node), si.name);

			bool pre = (si.symbol == S_PRE_INC || si.symbol == S_PRE_DEC);
			bool inc = (si.symbol == S_PRE_INC || si.symbol == S_POST_INC);
			Op oper = (inc ? INC : DEC);

			NodeRef op;
			op->pseudo_op = INTERNAL_GROUP;
			op->type = node->type;
			op->ref = REF_NO;
			op->source = nullptr;
			if(node->source)
				node->source->mod = true;

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
				case PUSH_INDEX:
					set_op = SET_INDEX;
					break;
				default:
					t.Throw("Operation '%s' require variable.", si.name);
				}

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
				else if(set_op == SET_INDEX)
				{
					// inc dec subscript
					assert(pnode->childs.size() == 2u); // push ar, push index
					op->push(pnode->childs[0]);
					op->push(PUSH);
					op->push(pnode->childs[1]);
					pnode->childs.clear();
					pnode->Free();
					op->push(PUSH);
					op->push(SWAP, 1);
					op->push(PUSH_INDEX);
					if(!pre)
						op->push(SET_TMP);
					op->push(oper);
					op->push(SET_INDEX);
					if(!pre)
					{
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
					op->push(INC);
					op->push(SET_ADR);
				}
				else
				{
					op->push(pnode);
					op->push(DEREF);
					op->push(pnode->copy());
					op->push(PUSH);
					op->push(DEREF);
					op->push(INC);
					op->push(SET_ADR);
					op->push(POP);
				}
			}

			stack.push_back(op.Pin());
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
			if(left->type == V_VOID)
				t.Throw("Invalid member access for type 'void'.");
			Type* type = GetType(left->type);
			if(right->pseudo_op == OBJ_FUNC)
			{
				vector<AnyFunction> funcs;
				FindAllFunctionOverloads(type, *right->str, funcs);
				if(funcs.empty())
					t.Throw("Missing method '%s' for type '%s'.", right->str->c_str(), type->name.c_str());
				StringPool.Free(right->str);

				NodeRef node = ParseNode::Get();
				node->ref = REF_NO;
				node->source = left->source;
				node->push(left.Pin());
				for(ParseNode* n : right->childs)
					node->push(n);
				right->childs.clear();

				ApplyFunctionCall(node, funcs, type, false);

				right.Pin()->Free();
				stack.push_back(node.Pin());
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
				node->source = left->source;
				node->push(left.Pin());
				right.Pin()->Free();
				stack.push_back(node);
			}
		}
		else if(si.type == ST_ASSIGN)
		{
			// assign to variable
			if(left->op != PUSH_LOCAL && left->op != PUSH_GLOBAL && left->op != PUSH_ARG && left->op != PUSH_MEMBER && left->op != PUSH_THIS_MEMBER
				&& left->op != PUSH_INDEX && left->ref != REF_YES)
				t.Throw("Can't assign, left value must be variable.");
			if(left->source)
				left->source->mod = true;

			NodeRef set;
			set->source = nullptr;
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
				case PUSH_INDEX:
					set->op = SET_INDEX;
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
					if(!TryCast(right.Get(), VarType(left->type)))
						t.Throw("Can't assign '%s' to type '%s'.", GetTypeName(right), GetTypeName(set));
					if(left->op == PUSH_MEMBER || left->op == PUSH_INDEX)
					{
						set->push(left->childs);
						left->childs.clear();
					}
					set->push(right.Pin());
					left.Pin()->Free();
				}
				else
				{
					// compound assign, left op= right
					VarType cast;
					int result;
					if(!CanOp((SYMBOL)si.op, left->GetVarType(), right->GetVarType(), cast, result))
						t.Throw("Invalid types '%s' and '%s' for operation '%s'.", GetTypeName(left), GetTypeName(right), si.name);

					NodeRef op;
					op->op = (Op)symbols[si.op].op;
					op->type = result;
					op->ref = REF_NO;
					op->source = nullptr;

					if(left->op == PUSH_MEMBER)
					{
						set->push(left->childs);
						set->push(PUSH);
						left->childs.clear();
						Cast(left.Get(), cast);
						Cast(right.Get(), cast);
						set->push(left.Pin());
						set->push(right.Pin());
						ForceCast(op.Get(), set, si.name);
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
						Cast(left.Get(), cast);
						Cast(right.Get(), cast);
						op->push(left.Pin());
						op->push(right.Pin());
						ForceCast(op.Get(), set, si.name);
						set->push(op.Pin());
					}
					else
					{
						Cast(left.Get(), cast);
						Cast(right.Get(), cast);

						op->push(left.Pin());
						op->push(right.Pin());

						ForceCast(op.Get(), set, si.name);
						set->push(op.Pin());
					}
				}
			}
			else
			{
				set->type = left->type;
				set->ref = REF_NO;

				if(si.op == NOP)
				{
					// assign
					if(!TryCast(right.Get(), VarType(left->type)))
						t.Throw("Can't assign '%s' to type '%s'.", GetTypeName(right), GetTypeName(left));
					set->op = SET_ADR;
					set->push(left.Pin());
					set->push(right.Pin());
				}
				else
				{
					// compound assign
					VarType cast;
					int result;
					if(!CanOp((SYMBOL)si.op, left->GetVarType(), right->GetVarType(), cast, result))
						t.Throw("Invalid types '%s' and '%s' for operation '%s'.", GetTypeName(left), GetTypeName(right), si.name);

					NodeRef real_left = left.Get()->copy();
					Cast(left.Get(), cast);
					Cast(right.Get(), cast);

					NodeRef op;
					op->op = (Op)symbols[si.op].op;
					op->type = result;
					op->ref = REF_NO;
					op->source = nullptr;
					op->push(left.Pin());
					op->push(right.Pin());

					ForceCast(op.Get(), set, si.name);
					set->push(real_left.Pin());
					set->push(op.Pin());
					set->op = SET_ADR;
				}
			}

			stack.push_back(set.Pin());
		}
		else
		{
			// normal operation
			VarType cast;
			int result;
			if(!CanOp(si.symbol, left->GetVarType(), right->GetVarType(), cast, result))
				t.Throw("Invalid types '%s' and '%s' for operation '%s'.", GetTypeName(left), GetTypeName(right), si.name);

			ParseNode* pleft = left.Pin();
			ParseNode* pright = right.Pin();

			Cast(pleft, cast);
			Cast(pright, cast);

			ParseNode* op = ParseNode::Get();
			op->type = result;
			op->ref = REF_NO;
			op->source = nullptr;

			if(!TryConstExpr(pleft, pright, op, si.symbol))
			{
				op->op = (Op)si.op;
				op->push(pleft);
				op->push(pright);
			}

			stack.push_back(op);
		}
	}
}

void Parser::ParseArgs(vector<ParseNode*>& nodes)
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

ParseNode* Parser::ParseItem(int* type)
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
		Type* rtype = GetType(var_type.core);
		if(!IS_SET(rtype->flags, Type::HaveCtor))
			t.Throw("Type '%s' don't have constructor.", rtype->name.c_str());
		NodeRef node;
		node->ref = REF_NO;
		node->source = nullptr;
		ParseArgs(node->childs);
		vector<AnyFunction> funcs;
		FindAllCtors(rtype, funcs);
		ApplyFunctionCall(node, funcs, rtype, true);
		return node.Pin();
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

				NodeRef node;
				node->ref = REF_NO;
				node->source = nullptr;

				ParseArgs(node->childs);
				ApplyFunctionCall(node, funcs, nullptr, false);

				return node.Pin();
			}
		case F_MEMBER:
			{
				ParseNode* node = ParseNode::Get();
				node->op = PUSH_THIS_MEMBER;
				node->type = found.member->type;
				node->value = found.member_index;
				node->ref = REF_MAY;
				node->source = nullptr;
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

ParseNode* Parser::ParseConstItem()
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
		node->type = V_FLOAT;
		node->fvalue = val;
		node->ref = REF_NO;
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
		node->type = V_CHAR;
		node->cvalue = c;
		node->ref = REF_NO;
		node->source = nullptr;
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
		node->source = nullptr;
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
		node->source = nullptr;
		return node;
	}
	else
		t.Unexpected();
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

VarType Parser::GetVarType(bool in_cpp)
{
	/*bool is_const = false;
	if(t.IsKeyword(K_CONST, G_KEYWORD))
	{
		is_const = true;
		t.Next();
	}*/
	if(!t.IsKeywordGroup(G_VAR))
		t.Unexpected("Expecting var type.");
	int type = t.GetKeywordId(G_VAR);
	//VarType type;
	//type.special = SV_NORMAL;
	//type.core = t.GetKeywordId(G_VAR);
	t.Next();
	if(t.IsSymbol('&'))
	{
		Type* ty = GetType(type);
		if(ty->IsRef())
			t.Throw("Can't create reference to reference type '%s'.", ty->name.c_str());
		t.Next();
		return VarType(type, SV_REF);
	}
	/*else if(t.IsSymbol('*'))
	{

	}
	else if(t.IsSymbol('?'))
	{

	}

	if(is_const)
	{
		switch(type.special)
		{
		case SV_NORMAL:
			type.special = SV_CONST;
			break;
		case SV_REF:
			type.special = SV_CONST_REF;
			break;
		case SV_PTR:
			type.special = SV_CONST_PTR;
			break;
		default:
			assert(0);
			break;
		}
	}*/
	
	return VarType(type);
}

int Parser::GetVarTypeForMember()
{
	VarType type = GetVarType();
	if(type.core == V_VOID)
		t.Throw("Member of 'void' type not allowed.");
	else
	{
		cstring name = nullptr;
		if(type.special == SV_REF)
			name = "reference";
		//else if(type.special == SV_ARRAY)
		//	name = "array";
		else if(type.core == V_STRING)
			name = "string";
		else
		{
			Type* ptype = GetType(type.core);
			if(ptype->IsClass())
			{
				if(ptype->IsRef())
					name = "class";
				else
					name = "struct";
			}
		}
		if(name)
			t.Throw("Member of '%s' type not allowed yet.", name);
	}
	return type.core;
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

BASIC_SYMBOL Parser::GetSymbol()
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
	case '[':
		return BS_SUBSCRIPT;
	case '~':
		return BS_BIT_NOT;
	default:
		return BS_MAX;
	}
}

bool Parser::CanOp(SYMBOL symbol, VarType leftvar, VarType rightvar, VarType& castvar, int& result)
{
	int& cast = castvar.core;
	castvar.special = SV_NORMAL;
	int left = leftvar.core;
	int right = rightvar.core;
	if(left == V_VOID)
		return false;
	if(right == V_VOID && symbols[symbol].args != 1)
		return false;
	Type* ltype = GetType(left);
	Type* rtype = GetType(right);
	if((ltype->IsClass() || rtype->IsClass()) && symbol != S_IS)
		return false;

	int type;
	switch(symbol)
	{
	case S_ADD:
		if(left == V_STRING || right == V_STRING)
			type = V_STRING;
		else if(left == V_FLOAT || right == V_FLOAT)
			type = V_FLOAT;
		else // int or char or bool
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
		else // int or char or bool
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
		else if(left == V_BOOL || left == V_CHAR)
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
		{
			if(left == V_CHAR || right == V_CHAR)
				type = V_CHAR;
			else
			{
				assert(left == V_BOOL && right == V_BOOL);
				type = V_BOOL;
			}
		}
		else
			type = V_INT;
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
		else if(ltype->IsClass() && left == right)
		{
			cast = left;
			result = V_BOOL;
			return true;
		}
		else if(leftvar.special == SV_REF && rightvar.special == SV_REF && left == right)
		{
			cast = left;
			castvar.special = SV_REF;
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
				op->type = V_CHAR;
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
				op->type = V_INT;
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
				op->type = V_FLOAT;
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

void Parser::Cast(ParseNode*& node, VarType type)
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
		deref->source = nullptr;
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
		cast->source = nullptr;
		cast->push(node);
		node = cast;
	}
}

// used in var assignment, passing argument to function
bool Parser::TryCast(ParseNode*& node, VarType type)
{
	int c = MayCast(node, type);
	if(c == 0)
		return true;
	else if(c == -1)
		return false;
	Cast(node, type);
	return true;
}

bool Parser::TryConstCast(ParseNode* node, int type)
{
	// can cast only const literal
	if(type == V_STRING)
		return false;

#define COMBINE(x,y) (((x)&0xFF)|(((y)&0xFF)<<8))

	switch(node->op)
	{
	case COMBINE(PUSH_BOOL,V_CHAR):
		node->cvalue = (node->bvalue ? 't' : 'f');
		node->op = PUSH_CHAR;
		node->type = V_CHAR;
		return true;
	case COMBINE(PUSH_BOOL,V_INT):
		node->value = (node->bvalue ? 1 : 0);
		node->op = PUSH_INT;
		node->type = V_INT;
		return true;
	case COMBINE(PUSH_BOOL,V_FLOAT):
		node->fvalue = (node->bvalue ? 1.f : 0.f);
		node->op = PUSH_FLOAT;
		node->type = V_FLOAT;
		return true;
	case COMBINE(PUSH_CHAR,V_BOOL):
		node->bvalue = (node->cvalue != 0);
		node->pseudo_op = PUSH_BOOL;
		node->type = V_BOOL;
		return true;
	case COMBINE(PUSH_CHAR,V_INT):
		node->value = (int)node->cvalue;
		node->op = PUSH_INT;
		node->type = V_INT;
		return true;
	case COMBINE(PUSH_CHAR,V_FLOAT):
		node->fvalue = (float)node->cvalue;
		node->op = PUSH_FLOAT;
		node->type = V_FLOAT;
		return true;
	case COMBINE(PUSH_INT,V_BOOL):
		node->bvalue = (node->value != 0);
		node->pseudo_op = PUSH_BOOL;
		node->type = V_BOOL;
		return true;
	case COMBINE(PUSH_INT,V_CHAR):
		node->cvalue = (char)node->value;
		node->op = PUSH_CHAR;
		node->type = V_CHAR;
		return true;
	case COMBINE(PUSH_INT,V_FLOAT):
		node->fvalue = (float)node->value;
		node->op = PUSH_FLOAT;
		node->type = V_FLOAT;
		return true;
	case COMBINE(PUSH_FLOAT,V_BOOL):
		node->bvalue = (node->fvalue != 0.f);
		node->pseudo_op = PUSH_BOOL;
		node->type = V_BOOL;
		return true;
	case COMBINE(PUSH_FLOAT,V_CHAR):
		node->cvalue = (char)node->fvalue;
		node->op = PUSH_CHAR;
		node->type = V_CHAR;
		return true;
	case COMBINE(PUSH_FLOAT,V_INT):
		node->value = (int)node->fvalue;
		node->op = PUSH_INT;
		node->type = V_INT;
		return true;
	default:
		return false;
	}

#undef COMBINE
}

// -1 - can't cast, 0 - no cast required, 1 - can cast
int Parser::MayCast(ParseNode* node, VarType type)
{
	// can't cast from void
	if(node->type == V_VOID)
		return -1;

	// no implicit cast from string to bool/char/int/float
	if(node->type == V_STRING && (type.core == V_BOOL || type.core == V_CHAR || type.core == V_INT || type.core == V_FLOAT))
		return -1;

	bool cast = (node->type != type.core);
	// can't cast class
	if(cast && (GetType(node->type)->IsClass() || GetType(type.core)->IsClass()))
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

void Parser::ForceCast(ParseNode*& node, ParseNode* type, cstring op)
{
	if(!TryCast(node, VarType(type->type)))
		t.Throw("Can't cast return value from '%s' to '%s' for operation '%s'.", GetTypeName(node), GetTypeName(type), op);
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
				ParseNode * not = ParseNode::Get();
				not->op = NOT;
				not->type = V_BOOL;
				not->ref = REF_NO;
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
	CheckGlobalReturnValue();
	for(ParseFunction* ufunc : ufuncs)
		VerifyFunctionReturnValue(ufunc);
}

void Parser::CheckGlobalReturnValue()
{
	if(global_returns.empty())
	{
		global_result = V_VOID;
		return;
	}

	int common = GetReturnType(global_returns[0].node);

	// verify common type
	for(uint i = 1; i < global_returns.size(); ++i)
	{
		ReturnInfo& info = global_returns[i];
		int other_type = GetReturnType(info.node);
		int new_common = CommonType(common, other_type);
		if(new_common == -1)
			t.ThrowAt(info.line, info.charpos, "Mismatched return type '%s' and '%s'.", GetType(common)->name.c_str(), GetType(other_type)->name.c_str());
		common = new_common;
	}

	// cast to common type
	for(ReturnInfo& info : global_returns)
	{
		int type = GetReturnType(info.node);
		if(type != common)
		{
			ParseNode* ret = info.node;
			ParseNode* expr = info.node->childs[0];
			ParseNode* cast = ParseNode::Get();
			cast->push(expr);
			cast->op = CAST;
			cast->type = common;
			cast->value = common;
			cast->ref = REF_NO;
			cast->source = nullptr;
			ret->childs[0] = cast;
		}
	}

	global_result = (CoreVarType)common;
}

void Parser::VerifyFunctionReturnValue(ParseFunction* f)
{
	if(f->result.core == V_VOID || f->special == SF_CTOR)
		return;

	if(f->node)
	{
		if(f->node->pseudo_op == RETURN)
			return;

		for(vector<ParseNode*>::reverse_iterator it = f->node->childs.rbegin(), end = f->node->childs.rend(); it != end; ++it)
		{
			if(VerifyNodeReturnValue(*it))
				return;
		}
	}

	t.Throw("%s '%s' not always return value.", f->type == V_VOID ? "Function" : "Method", GetName(f));
}

bool Parser::VerifyNodeReturnValue(ParseNode* node)
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

void Parser::CopyFunctionChangedStructs()
{
	for(ParseFunction* f : ufuncs)
	{
		ParseNode* node = nullptr;
		for(ParseVar* local : f->args)
		{
			if(local->mod && GetType(local->type.core)->IsStruct())
			{
				if(!node)
				{
					node = ParseNode::Get();
					node->pseudo_op = INTERNAL_GROUP;
					node->type = V_VOID;
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
	for(ParseFunction* ufunc : ufuncs)
	{
		ufunc->pos = run_module->code.size();
		ufunc->locals = ufunc->block->GetMaxVars();
		uint old_size = run_module->code.size();
		if(ufunc->node)
			ToCode(run_module->code, ufunc->node, nullptr);
		if(old_size == run_module->code.size() || run_module->code.back() != RET)
			run_module->code.push_back(RET);
	}
	run_module->entry_point = run_module->code.size();
	uint old_size = run_module->code.size();
	ToCode(run_module->code, global_node, nullptr);
	if(old_size == run_module->code.size() || run_module->code.back() != RET)
		run_module->code.push_back(RET);
}

void Parser::ToCode(vector<int>& code, ParseNode* node, vector<uint>* break_pos)
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
	case CALL:
	case CALLU:
		code.push_back(node->op);
		code.push_back(node->value);
		if(node->source && node->source->mod && node->source->index == -1)
			code.push_back(COPY);
		break;
	case PUSH_INT:
	case PUSH_STRING:
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
	case COPY_ARG:
	case SWAP:
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
	case SET_INDEX:
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

Type* Parser::GetType(int index)
{
	int module_index = (index & 0xFFFF0000) >> 16;
	int type_index = (index & 0xFFFF);
	if(module_index == 0xFFFF)
	{
		assert(type_index < (int)run_module->types.size());
		return run_module->types[type_index];
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

int Parser::GetReturnType(ParseNode* node)
{
	if(node->childs.empty())
		return V_VOID;
	else
		return node->childs.front()->type;
}

cstring Parser::GetName(ParseVar* var)
{
	assert(var);
	return Format("%s %s", GetType(var->type.core)->name.c_str(), var->name.c_str());
}

cstring Parser::GetName(CommonFunction* cf, bool write_result, bool write_type, BASIC_SYMBOL* symbol)
{
	assert(cf);
	LocalString s = "";
	if(write_result && cf->special != SF_CTOR)
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
		++var_offset;
	}
	if(!symbol)
		s += cf->name;
	else
	{
		s += "operator ";
		s += basic_symbols[*symbol].text;
		s += ' ';
	}
	s += '(';
	for(uint i = var_offset, count = cf->arg_infos.size(); i < count; ++i)
	{
		if(i != var_offset)
			s += ",";
		s += GetName(cf->arg_infos[i].type);
	}
	s += ")";
	return Format("%s", s->c_str());
}

cstring Parser::GetName(VarType type)
{
	Type* t = GetType(type.core);
	if(type.special == SV_NORMAL)
		return t->name.c_str();
	else
		return Format("%s&", t->name.c_str());
}

cstring Parser::GetTypeName(ParseNode* node)
{
	return GetName(node->GetVarType());
}

cstring Parser::GetParserFunctionName(uint index)
{
	assert(index < ufuncs.size());
	return GetName(ufuncs[index], false);
}

int Parser::CommonType(int a, int b)
{
	if(a == b)
		return a;
	if(a == V_VOID || b == V_VOID)
		return -1;
	if(a == V_FLOAT || b == V_FLOAT)
		return V_FLOAT;
	else if(a == V_INT || b == V_INT)
		return V_INT;
	else if(a == V_CHAR || b == V_CHAR)
		return V_CHAR;
	else
	{
		assert(0);
		return -1;
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

AnyFunction Parser::FindEqualFunction(Type* type, Function& fc)
{
	assert(type);

	if(fc.special <= SF_CTOR)
	{
		for(Function* f : type->funcs)
		{
			if(f->name == fc.name && f->Equal(fc))
				return f;
		}

		for(ParseFunction* pf : type->ufuncs)
		{
			if(pf->name == fc.name && pf->Equal(fc))
				return pf;
		}
	}
	else
	{
		for(Function* f : type->funcs)
		{
			if(f->special == fc.special)
				return f;
		}

		for(ParseFunction* pf : type->ufuncs)
		{
			if(pf->special == fc.special)
				return pf;
		}
	}

	return nullptr;
}

// 0 - don't match, 1 - require cast, 2 - match
int Parser::MatchFunctionCall(ParseNode* node, CommonFunction& f, bool is_parse)
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

void Parser::ApplyFunctionCall(ParseNode* node, vector<AnyFunction>& funcs, Type* type, bool ctor)
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
				s += GetType(node->childs[i]->type)->name;
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
		bool callu_ctor = false;

		if(current_type && cf.type == current_type->index)
		{
			// push this
			ParseNode* thi = ParseNode::Get();
			thi->op = PUSH_ARG;
			thi->type = cf.type;
			thi->value = 0;
			thi->ref = REF_NO;
			thi->source = nullptr;
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
			n->source = nullptr;
			assert(arg.type.special == SV_NORMAL);
			switch(arg.type.core)
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

		if(GetType(node->type)->IsStruct())
		{
			ReturnStructVar* rsv = new ReturnStructVar;
			rsv->index = -1;
			rsv->node = node;
			rsv->mod = false;
			rsvs.push_back(rsv);
			node->source = rsv;
		}
	}
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
			if(si.args - 1 == f.arg_infos.size() || s == S_SUBSCRIPT)
			{
				f.name = si.op_code;
				return true;
			}
		}
	}
	return false;
}
