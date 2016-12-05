#pragma once

#include "Function.h"
#include "RunModule.h"

struct Block;
struct CastResult;
struct ParseFunction;
struct ParseNode;
struct ParseVar;
struct SymbolNode;
struct ReturnStructVar;
union Found;
enum BASIC_SYMBOL;
enum FOUND;
enum RETURN_INFO;
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

	bool VerifyTypeName(cstring type_name, int& type_index);
	Function* ParseFuncDecl(cstring decl, Type* type);
	Member* ParseMemberDecl(cstring decl);
	void AddType(Type* type);
	Function* GetFunction(int index);
	Type* GetType(int index);
	cstring GetName(CommonFunction* cf, bool write_result = true, bool write_type = true, BASIC_SYMBOL* symbol = nullptr);
	cstring GetParserFunctionName(uint index);
	RunModule* Parse(ParseSettings& settigns);
	void Cleanup();
	AnyFunction FindEqualFunction(Type* type, AnyFunction f);

private:
	void FinishRunModule();
	void AddKeywords();
	void AddChildModulesKeywords();

	void ParseCode();
	ParseNode* ParseLineOrBlock();
	ParseNode* ParseBlock(ParseFunction* f = nullptr);
	ParseNode* ParseLine();
	ParseNode* ParseSwitch();
	ParseNode* ParseCase(ParseNode* swi);
	void ParseMemberDeclClass(Type* type, uint& pad);
	void ParseFuncInfo(CommonFunction* f, Type* type, bool in_cpp);
	void ParseFunctionArgs(CommonFunction* f, bool in_cpp);
	ParseNode* ParseVarTypeDecl();
	ParseNode* ParseCond();
	ParseNode* ParseVarDecl(VarType type);
	ParseNode* ParseExpr(char end, char end2 = 0, int* type = nullptr);
	void ParseExprConvertToRPN(vector<SymbolNode>& exit, vector<SymbolNode>& stack, int* type);
	BASIC_SYMBOL ParseExprPart(vector<SymbolNode>& exit, vector<SymbolNode>& stack, int* type);
	void ParseExprPartPost(BASIC_SYMBOL& symbol, vector<SymbolNode>& exit, vector<SymbolNode>& stack);
	void ParseExprApplySymbol(vector<ParseNode*>& stack, SymbolNode& sn);
	void ParseArgs(vector<ParseNode*>& nodes, char open = '(', char close = ')');
	ParseNode* ParseItem(int* type = nullptr);
	ParseNode* ParseConstItem();

	void CheckFindItem(const string& id, bool is_func);
	ParseVar* GetVar(ParseNode* node);
	VarType GetVarType(bool in_cpp = false);
	int GetVarTypeForMember();
	void PushSymbol(SYMBOL symbol, vector<SymbolNode>& exit, vector<SymbolNode>& stack, ParseNode* node = nullptr);
	bool GetNextSymbol(BASIC_SYMBOL& symbol);
	BASIC_SYMBOL GetSymbol(bool full_over = false);
	bool CanOp(SYMBOL symbol, ParseNode* lnode, ParseNode* rnode, VarType& cast, int& result, ParseNode*& over_result, SYMBOL real_symbol);
	bool TryConstExpr(ParseNode* left, ParseNode* right, ParseNode* op, SYMBOL symbol);
	bool TryConstExpr1(ParseNode* node, SYMBOL symbol);

	void Cast(ParseNode*& node, VarType type, CastResult* cast_result = nullptr, bool implici = true);
	bool TryCast(ParseNode*& node, VarType type, bool implici = true);
	bool TryConstCast(ParseNode* node, int type);
	CastResult MayCast(ParseNode* node, VarType type);
	void ForceCast(ParseNode*& node, ParseNode* type, cstring op);

	void Optimize();
	ParseNode* OptimizeTree(ParseNode* node);

	void CheckReturnValues();
	void CheckGlobalReturnValue();
	void VerifyFunctionReturnValue(ParseFunction* f);
	RETURN_INFO VerifyNodeReturnValue(ParseNode* node, bool in_switch);

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
	AnyFunction FindSpecialFunction(Type* type, SpecialFunction spec, delegate<bool(AnyFunction& f)> pred);
	int MatchFunctionCall(ParseNode* node, CommonFunction& f, bool is_parse);
	void ApplyFunctionCall(ParseNode* node, vector<AnyFunction>& funcs, Type* type, bool ctor);
	bool CanOverload(BASIC_SYMBOL symbol);
	bool FindMatchingOverload(CommonFunction& f, BASIC_SYMBOL symbol);
	int GetNextType(); // 0-var, 1-ctor, 2-func, 3-operator, 4-type

	void AnalyzeCode();
	void AnalyzeType(Type* type);
	ParseFunction* AnalyzeArgs(VarType result, SpecialFunction special, Type* type, cstring name);
	VarType AnalyzeVarType();
	Type* AnalyzeAddType(const string& name);

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
	vector<ReturnStructVar*> rsvs;
};
