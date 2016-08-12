#include "Pch.h"
#include "Base.h"
#include "CasImpl.h"
#include "Parser.h"
#include "ParserImpl.h"
#include "Module.h"

Parser::Parser(Module* module) : module(module), t(Tokenizer::F_SEEK | Tokenizer::F_UNESCAPE)
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
			if(!type->hidden)
				AddType(type);
		}
	}
}

bool Parser::Parse(ParseContext& ctx)
{
	optimize = ctx.optimize;
	t.FromString(ctx.input);

	try
	{
		ParseCode();
		Optimize();
		CheckReturnValues();
		ConvertToBytecode();
		ApplyParseContextResult(ctx);
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		handler(cas::Error, e.ToString());
		return false;
	}
}

void Parser::ApplyParseContextResult(ParseContext& ctx)
{
	ctx.strs = strs;
	ctx.ufuncs.resize(ufuncs.size());
	for(uint i = 0; i < ufuncs.size(); ++i)
	{
		ParseFunction& f = *ufuncs[i];
		UserFunction& uf = ctx.ufuncs[i];
		uf.pos = f.pos;
		uf.locals = f.locals;
		uf.result = f.result;
		for(auto& arg : f.arg_infos)
			uf.args.push_back(arg.type);
		uf.type = f.type;
	}
	ctx.globals = main_block->GetMaxVars();
	ctx.result = global_result;
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
	global_returns.clear();

	ParseNode* node = ParseNode::Get();
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

	return node;
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
					t.Throw("Condition expression with '%s' type.", types[for2->type]->name.c_str());

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
							type_name = types[V_VOID]->name.c_str();
						}
					}
					else
					{
						ParseNode*& r = ret->childs[0];
						if(!TryCast(r, req_type))
						{
							ok = false;
							type_name = r->GetVarType().GetName();
						}

						if(ok && current_function->special == SF_NO && current_function->result.special == SV_REF)
						{
							if(r->op == PUSH_LOCAL_REF || r->op == PUSH_ARG_REF)
								t.Throw("Returning reference to temporary variable '%s'.", GetVar(r)->GetName());
						}
					}

					if(!ok)
						t.Throw("Invalid return type '%s', %s '%s' require '%s' type.", type_name, current_function->type == V_VOID ? "function" : "method",
							current_function->GetName(), req_type.GetName());
				}
				else
				{
					if(!ret->childs.empty())
					{
						ParseNode* r = ret->childs[0];
						if((r->type != V_INT && r->type != V_BOOL && r->type != V_FLOAT) || r->ref == REF_YES)
							t.Throw("Invalid type '%s' for global return.", r->GetVarType().GetName());
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
				type->have_ctor = false;
				type->index = types.size();
				type->pod = true;
				type->is_ref = true;
				type->size = 0;
				types.push_back(type);
				AddParserType(type);
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
							t.Throw("Function '%s' already exists.", f->GetName());

						// block
						f->node = ParseBlock(f);
						current_function = nullptr;
						if(f->special == SF_CTOR)
							type->have_ctor = true;
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
				cstring name = f->GetName();
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
		handler(cas::Error, e.ToString());
		delete m;
		m = nullptr;
	}

	return m;
}

void Parser::ParseMemberDeclClass(Type* type, uint& pad)
{
	int var_type = GetVarTypeForMember();
	uint var_size = types[var_type]->size;
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
					t.Throw("Invalid default value of type '%s', required '%s'.", types[item->type]->name.c_str(), type.GetName());
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

ParseNode* Parser::ParseVarTypeDecl(int* _type = nullptr, string* _name = nullptr)
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
		t.Throw("Condition expression with '%s' type.", types[cond->type]->name.c_str());
	t.Next();
	return cond;
}

ParseNode* ParseVarDecl(int type, string* _name)
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
		default:
			if(type >= V_CLASS)
			{
				Type* rtype = types[type];
				if(rtype->have_ctor)
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
			else
				t.Throw("Missing default value for type '%s'.", types[type]->name.c_str());
			break;
		}
	}
	else
	{
		t.Next();

		// expr<,;>
		expr = ParseExpr(',', ';');
		if(!TryCast(expr, VarType(type)))
			t.Throw("Can't assign type '%s' to variable '%s %s'.", types[expr->type]->name.c_str(), var->name.c_str(), types[type]->name.c_str());
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
		LoopAndRemove(node->childs, [](ParseNode*& node) {
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
			t.ThrowAt(info.line, info.charpos, "Mismatched return type '%s' and '%s'.", types[common]->name.c_str(), types[other_type]->name.c_str());
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

	t.Throw("%s '%s' not always return value.", f->type == V_VOID ? "Function" : "Method", f->GetName());
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
		ufunc->pos = ctx.code.size();
		ufunc->locals = ufunc->block->GetMaxVars();
		uint old_size = ctx.code.size();
		if(ufunc->node)
			ToCode(ctx.code, ufunc->node, nullptr);
		if(old_size == ctx.code.size() || ctx.code.back() != RET)
			ctx.code.push_back(RET);
	}
	ctx.entry_point = ctx.code.size();
	uint old_size = ctx.code.size();
	ToCode(ctx.code, node, nullptr);
	if(old_size == ctx.code.size() || ctx.code.back() != RET)
		ctx.code.push_back(RET);
}
