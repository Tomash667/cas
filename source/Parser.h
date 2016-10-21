#pragma once

#include "Function.h"
#include "RunModule.h"

struct Block;
struct ParseFunction;
struct ParseNode;
struct ParseVar;
struct SymbolOrNode;
union Found;
enum BASIC_SYMBOL;
enum FOUND;
enum SYMBOL;

struct ParseSettings
{
	string input;
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
	Function* GetFunction(int index);
	Type* GetType(int index);
	cstring GetName(CommonFunction* cf, bool write_result = true);
	cstring GetParserFunctionName(uint index);
	RunModule* Parse(ParseSettings& settigns);
	void Cleanup();
	AnyFunction FindEqualFunction(Type* type, Function& fc);

private:
	void FinishRunModule();
	void AddKeywords();
	void AddChildModulesKeywords();

	void ParseCode();
	ParseNode* ParseLineOrBlock();
	ParseNode* ParseBlock(ParseFunction* f = nullptr);
	ParseNode* ParseLine();
	void ParseMemberDeclClass(Type* type, uint& pad);
	void ParseFunctionArgs(CommonFunction* f, bool real_func);
	ParseNode* ParseVarTypeDecl(int* _type = nullptr, string* _name = nullptr);
	ParseNode* ParseCond();
	ParseNode* ParseVarDecl(int type, string* _name);
	ParseNode* ParseExpr(char end, char end2 = 0, int* type = nullptr);
	BASIC_SYMBOL ParseExprPart(vector<SymbolOrNode>& exit, vector<SYMBOL>& stack, int* type);
	void ParseArgs(vector<ParseNode*>& nodes);
	ParseNode* ParseItem(int* type = nullptr);
	ParseNode* ParseConstItem();

	void CheckFindItem(const string& id, bool is_func);
	ParseVar* GetVar(ParseNode* node);
	VarType GetVarType(bool in_cpp = false);
	int GetVarTypeForMember();
	void PushSymbol(SYMBOL symbol, vector<SymbolOrNode>& exit, vector<SYMBOL>& stack);
	bool GetNextSymbol(BASIC_SYMBOL& symbol);
	BASIC_SYMBOL GetSymbol();
	bool CanOp(SYMBOL symbol, VarType leftvar, VarType rightvar, VarType& cast, int& result);
	bool TryConstExpr(ParseNode* left, ParseNode* right, ParseNode* op, SYMBOL symbol);
	bool TryConstExpr1(ParseNode* node, SYMBOL symbol);

	void Cast(ParseNode*& node, VarType type);
	bool TryCast(ParseNode*& node, VarType type);
	bool TryConstCast(ParseNode* node, int type);
	int MayCast(ParseNode* node, VarType type);

	void Optimize();
	ParseNode* OptimizeTree(ParseNode* node);

	void CheckReturnValues();
	void CheckGlobalReturnValue();
	void VerifyFunctionReturnValue(ParseFunction* f);
	bool VerifyNodeReturnValue(ParseNode* node);

	void CopyFunctionChangedStructs();
	void ConvertToBytecode();
	void ToCode(vector<int>& code, ParseNode* node, vector<uint>* break_pos);

	int GetReturnType(ParseNode* node);
	cstring GetName(ParseVar* var);
	cstring GetName(VarType type);
	cstring GetTypeName(ParseNode* node);
	int CommonType(int a, int b);
	FOUND FindItem(const string& id, Found& found);
	Function* FindFunction(const string& name);
	AnyFunction FindFunction(Type* type, const string& name);
	void FindAllFunctionOverloads(const string& name, vector<AnyFunction>& items);
	void FindAllFunctionOverloads(Type* type, const string& name, vector<AnyFunction>& funcs);
	AnyFunction FindEqualFunction(ParseFunction* pf);
	void FindAllCtors(Type* type, vector<AnyFunction>& funcs);
	int MatchFunctionCall(ParseNode* node, CommonFunction& f, bool is_parse);
	void ApplyFunctionCall(ParseNode* node, vector<AnyFunction>& funcs, Type* type, bool ctor);

	Tokenizer t;
	Module* module;
	RunModule* run_module;
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
