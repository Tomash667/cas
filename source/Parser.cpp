#include "Pch.h"
#include "CasImpl.h"
#include "Parser.h"
#include "ParserImpl.h"
#include "Module.h"

// http://en.cppreference.com/w/cpp/language/operator_precedence
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

Parser::Parser(Module* module) : module(module), t(Tokenizer::F_SEEK | Tokenizer::F_UNESCAPE), run_module(nullptr)
{
	AddKeywords();
	AddChildModulesKeywords();
}

bool Parser::VerifyTypeName(cstring type_name)
{
	assert(type_name);
	t.CheckItemOrKeyword(type_name);
	return !t.IsKeyword();
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
		{ "is", K_IS }
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
	else
	{
		ParseNode* node = ParseNode::Get();
		node->pseudo_op = GROUP;
		node->type = V_VOID;
		node->ref = REF_NO;
		node->childs = nodes;
		return node;
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

				ParseNode* if_op = ParseNode::Get();
				if_op->pseudo_op = IF;
				if_op->type = V_VOID;
				if_op->ref = REF_NO;
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
				do_whil->ref = REF_NO;
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
				whil->ref = REF_NO;
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

				if(for2 && !TryCast(for2, VarType(V_BOOL)))
					t.Throw("Condition expression with '%s' type.", GetTypeName(for2));

				ParseNode* fo = ParseNode::Get();
				fo->pseudo_op = FOR;
				fo->type = V_VOID;
				fo->ref = REF_NO;
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
				br->ref = REF_NO;
				return br;
			}
		case K_RETURN:
			{
				ParseNode* ret = ParseNode::Get();
				ret->pseudo_op = RETURN;
				ret->type = V_VOID;
				ret->ref = REF_NO;
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
				return ret;
			}
		case K_CLASS:
			{
				if(current_block != main_block)
					t.Throw("Class can't be declared inside block.");

				// id
				t.Next();
				const string& id = t.MustGetItem();
				CheckFindItem(id, false);
				Type* type = new Type;
				type->name = id;
				type->flags = Type::Pod | Type::Ref | Type::Class;
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
						ParseFunctionArgs(f, true);
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
		VarType type((CoreVarType)t.GetKeywordId(G_VAR));
		t.Next();
		if(t.IsSymbol('('))
		{
			// ctor
			int var_type = type.core;
			ParseNode* node = ParseExpr(';', 0, &var_type);
			t.AssertSymbol(';');
			t.Next();

			return node;
		}
		else if(t.IsSymbol('&'))
		{
			type.special = SV_REF;
			t.Next();
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

			// args
			ParseFunctionArgs(f, true);
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
			if(type.special != SV_NORMAL)
				t.Throw("Reference variable unavailable yet.");
			int var_type = type.core;
			ParseNode* node = ParseVarTypeDecl(&var_type, str.get_ptr());
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

// func_decl
Function* Parser::ParseFuncDecl(cstring decl, Type* type)
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

void Parser::ParseFunctionArgs(CommonFunction* f, bool real_func)
{
	assert(f);
	f->required_args = 0;
	if(real_func)
		current_function = (ParseFunction*)f;

	// args
	bool prev_arg_def = false;
	if(!t.IsSymbol(')'))
	{
		while(true)
		{
			VarType type = GetVarType();
			LocalString id = t.MustGetItem();
			CheckFindItem(id.get_ref(), false);
			if(real_func)
			{
				ParseVar* arg = ParseVar::Get();
				arg->name = id;
				arg->type = type;
				arg->subtype = ParseVar::ARG;
				arg->index = current_function->args.size();
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

ParseNode* Parser::ParseVarTypeDecl(int* _type, string* _name)
{
	// var_type
	int type;
	if(_type)
		type = *_type;
	else
		type = t.GetKeywordId();
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
	else
	{
		ParseNode* node = ParseNode::Get();
		node->pseudo_op = GROUP;
		node->type = V_VOID;
		node->ref = REF_NO;
		node->childs = nodes;
		return node;
	}
}

ParseNode* Parser::ParseCond()
{
	t.AssertSymbol('(');
	t.Next();
	ParseNode* cond = ParseExpr(')');
	t.AssertSymbol(')');
	if(!TryCast(cond, VarType(V_BOOL)))
		t.Throw("Condition expression with '%s' type.", GetTypeName(cond));
	t.Next();
	return cond;
}

ParseNode* Parser::ParseVarDecl(int type, string* _name)
{
	// var_name
	const string& name = (_name ? *_name : t.MustGetItem());
	if(!_name)
		CheckFindItem(name, false);

	ParseVar* var = ParseVar::Get();
	var->name = name;
	var->type = VarType(type);
	var->index = current_block->var_offset;
	var->subtype = (current_function == nullptr ? ParseVar::GLOBAL : ParseVar::LOCAL);
	current_block->vars.push_back(var);
	current_block->var_offset++;
	if(!_name)
		t.Next();

	// [=]
	ParseNode* expr;
	if(!t.IsSymbol('='))
	{
		expr = ParseNode::Get();
		expr->ref = REF_NO;
		switch(type)
		{
		case V_BOOL:
			expr->type = V_BOOL;
			expr->pseudo_op = PUSH_BOOL;
			expr->bvalue = false;
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
				Type* rtype = GetType(type);
				if(IS_SET(rtype->flags, Type::HaveCtor))
				{
					vector<AnyFunction> funcs;
					FindAllCtors(rtype, funcs);
					ApplyFunctionCall(expr, funcs, rtype, true);
				}
				else
				{
					expr->type = type;
					expr->op = CTOR;
					expr->value = type;
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
		if(!TryCast(expr, VarType(type)))
			t.Throw("Can't assign type '%s' to variable '%s'.", GetTypeName(expr), GetName(var));
	}

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
	node->type = var->type.core;
	node->value = var->index;
	node->ref = REF_NO;
	node->push(expr);
	return node;
}

ParseNode* Parser::ParseExpr(char end, char end2, int* type)
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
					VarType cast;
					int result;
					if(!CanOp(sn.symbol, node->GetVarType(), VarType(V_VOID), cast, result))
						t.Throw("Invalid type '%s' for operation '%s'.", GetTypeName(node), si.name);
					Cast(node, cast);
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
						t.Throw("Invalid type '%s' for operation '%s'.", GetTypeName(node), si.name);

					bool pre = (si.symbol == S_PRE_INC || si.symbol == S_PRE_DEC);
					bool inc = (si.symbol == S_PRE_INC || si.symbol == S_POST_INC);
					Op oper = (inc ? INC : DEC);

					ParseNode* op = ParseNode::Get();
					op->pseudo_op = INTERNAL_GROUP;
					op->type = node->type;
					op->ref = REF_NO;

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
							set_adr; [a]+1  a->a+1
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
					Type* type = GetType(left->type);
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
								t.Throw("Can't assign '%s' to type '%s'.", GetTypeName(right), GetTypeName(set));
							set->push(right);
							if(left->op == PUSH_MEMBER)
							{
								set->push(left->childs);
								left->childs.clear();
							}
							left->Free();
						}
						else
						{
							// compound assign
							VarType cast;
							int result;
							if(!CanOp((SYMBOL)si.op, left->GetVarType(), right->GetVarType(), cast, result))
								t.Throw("Invalid types '%s' and '%s' for operation '%s'.", GetTypeName(left), GetTypeName(right), si.name);

							Cast(left, cast);
							Cast(right, cast);

							ParseNode* op = ParseNode::Get();
							op->op = (Op)symbols[si.op].op;
							op->type = result;
							op->ref = REF_NO;
							op->push(left);
							op->push(right);

							if(!TryCast(op, VarType(set->type)))
								t.Throw("Can't cast return value from '%s' to '%s' for operation '%s'.", GetTypeName(op), GetTypeName(set), si.name);
							set->push(op);
							if(left->op == PUSH_MEMBER)
								set->push_copy(left->childs);
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
								t.Throw("Can't assign '%s' to type '%s'.", GetTypeName(right), GetTypeName(left));
							set->op = SET_ADR;
							set->push(left);
							set->push(right);
						}
						else
						{
							// compound assign
							VarType cast;
							int result;
							if(!CanOp((SYMBOL)si.op, left->GetVarType(), right->GetVarType(), cast, result))
								t.Throw("Invalid types '%s' and '%s' for operation '%s'.", GetTypeName(left), GetTypeName(right), si.name);

							ParseNode* real_left = left;
							Cast(left, cast);
							Cast(right, cast);

							ParseNode* op = ParseNode::Get();
							op->op = (Op)symbols[si.op].op;
							op->type = result;
							op->ref = REF_NO;
							op->push(left);
							op->push(right);

							if(!TryCast(op, VarType(set->type)))
								t.Throw("Can't cast return value from '%s' to '%s' for operation '%s'.", GetTypeName(op), GetTypeName(set), si.name);
							set->push(real_left->copy());
							set->push(op);
							set->op = SET_ADR;
						}
					}

					stack2.push_back(set);
				}
				else
				{
					VarType cast;
					int result;
					if(!CanOp(si.symbol, left->GetVarType(), right->GetVarType(), cast, result))
						t.Throw("Invalid types '%s' and '%s' for operation '%s'.", GetTypeName(left), GetTypeName(right), si.name);

					Cast(left, cast);
					Cast(right, cast);

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

BASIC_SYMBOL Parser::ParseExprPart(vector<SymbolOrNode>& exit, vector<SYMBOL>& stack, int* type)
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

VarType Parser::GetVarType()
{
	if(!t.IsKeywordGroup(G_VAR))
		t.Unexpected("Expecting var type.");
	int type = t.GetKeywordId(G_VAR);
	t.Next();
	if(t.IsSymbol('&'))
	{
		Type* ty = GetType(type);
		if(IS_SET(ty->flags, Type::Ref))
			t.Throw("Can't create reference to reference type '%s'.", ty->name.c_str());
		t.Next();
		return VarType(type, SV_REF);
	}
	else
		return VarType(type);
}

int Parser::GetVarTypeForMember()
{
	int type_index = t.MustGetKeywordId(G_VAR);
	if(type_index == V_VOID)
		t.Throw("Class member can't be void type.");
	else
	{
		Type* type = GetType(type_index);
		if(type_index == V_STRING || type->IsClass())
			t.Throw("Class '%s' member not supported yet.", type->name.c_str());
	}
	t.Next();
	return type_index;
}

void Parser::PushSymbol(SYMBOL symbol, vector<SymbolOrNode>& exit, vector<SYMBOL>& stack)
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

// -1 - can't cast, 0 - no cast required, 1 - can cast
int Parser::MayCast(ParseNode* node, VarType type)
{
	// can't cast from void
	if(node->type == V_VOID)
		return -1;

	// no implicit cast from string to bool/int/float
	if(node->type == V_STRING && (type.core == V_BOOL || type.core == V_INT || type.core == V_FLOAT))
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

cstring Parser::GetName(CommonFunction* cf, bool write_result)
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
		s += GetType(cf->type)->name;
		s += '.';
		++var_offset;
	}
	s += cf->name;
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
