#pragma once

#include "Function.h"

struct Block;
struct ParseFunction;
struct ParseNode;

struct ParseContext
{
	string input;
	vector<Str*> strs;
	vector<int> code;
	vector<UserFunction> ufuncs;
	uint globals, entry_point;
	CoreVarType result;
	bool optimize;
};

struct ReturnInfo
{
	ParseNode* node;
	uint line, charpos;
};

class Parser
{
public:
	Parser(Module* module);

	bool VerifyTypeName(cstring type_name);
	Function* ParseFuncDecl(cstring decl, Type* type);
	Member* ParseMemberDecl(cstring decl);
	void AddType(Type* type);

	bool Parse(ParseContext& ctx);
	void ApplyParseContextResult(ParseContext& ctx);

	void ConvertToBytecode();

private:
	void AddKeywords();
	void AddChildModulesKeywords();

	void ParseCode();
	ParseNode* ParseLineOrBlock();
	ParseNode* ParseBlock(ParseFunction* f);
	ParseNode* ParseLine();
	Member* ParseMemberDecl(cstring decl);
	void ParseMemberDeclClass(Type* type, uint& pad);
	void ParseFunctionArgs(CommonFunction* f, bool real_func);
	ParseNode* ParseVarTypeDecl(int* _type = nullptr, string* _name = nullptr);
	ParseNode* ParseCond();
	ParseNode* ParseVarDecl(int type, string* _name);

	void Optimize();
	ParseNode* OptimizeTree(ParseNode* node);

	void CheckReturnValues();
	void CheckGlobalReturnValue();
	void VerifyFunctionReturnValue(ParseFunction* f);
	bool VerifyNodeReturnValue(ParseNode* node);

	Tokenizer t;
	Module* module;
	vector<Str*> strs;
	vector<ParseFunction*> ufuncs;
	vector<ReturnInfo> global_returns;
	Block* main_block, *current_block;
	ParseFunction* current_function;
	Type* current_type;
	ParseNode* global_node;
	int breakable_block, empty_string;
	CoreVarType global_result;
	bool optimize;
};
