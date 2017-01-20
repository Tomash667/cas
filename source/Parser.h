#pragma once

#include "Function.h"
#include "RunModule.h"

struct Block;
struct CastResult;
struct Enum;
struct OpResult;
struct ParseFunction;
struct ParseNode;
struct ParseVar;
struct SymbolNode;
struct ReturnStructVar;
struct SymbolInfo;
union Found;
enum BASIC_SYMBOL;
enum FOUND;
enum Op;
enum RETURN_INFO;
enum SYMBOL;
typedef ObjectPoolRef<ParseNode> NodeRef;

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

enum BuiltinFunc
{
	BF_ASSIGN = 1<<0,
	BF_EQUAL = 1<<1,
	BF_NOT_EQUAL = 1<<2
};

enum CastFlags
{
	CF_EXPLICIT = 1<<0,
	CF_PASS_BY_REF = 1<<1,
	CF_REQUIRE_CONST = 1<<2
};

class Parser
{
public:
	Parser(Module* module);
	~Parser();

	bool VerifyTypeName(cstring type_name, int& type_index);
	Function* ParseFuncDecl(cstring decl, Type* type);
	Member* ParseMemberDecl(cstring decl);
	void AddType(Type* type);
	Function* GetFunction(int index);
	Type* GetType(int index);
	cstring GetName(CommonFunction* cf, bool write_result = true, bool write_type = true, BASIC_SYMBOL* symbol = nullptr, int generic_type = V_VOID);
	cstring GetParserFunctionName(uint index);
	RunModule* Parse(ParseSettings& settigns);
	void Cleanup();
	AnyFunction FindEqualFunction(Type* type, AnyFunction f);
	int CreateDefaultFunctions(Type* type);

private:
	void FinishRunModule();
	void AddKeywords();
	void AddChildModulesKeywords();

	void ParseCode();
	ParseNode* ParseLineOrBlock();
	ParseNode* ParseBlock(ParseFunction* f = nullptr);
	ParseNode* ParseLine();
	ParseNode* ParseReturn();
	void ParseClass(bool is_struct);
	ParseNode* ParseSwitch();
	ParseNode* ParseCase(ParseNode* swi);
	void ParseEnum(bool forward);
	void ParseMemberDeclClass(Type* type, uint& pad);
	ParseNode* ParseFunc();
	void ParseFuncModifiers(bool have_type, int& flags);
	void ParseFuncInfo(CommonFunction* f, Type* type, bool in_cpp);
	void ParseFunctionArgs(CommonFunction* f, bool in_cpp, Type* type);
	ParseNode* ParseVarTypeDecl();
	ParseNode* ParseCond();
	ParseNode* ParseVarDecl(VarType vartype);
	ParseNode* ParseExpr(char end, char end2 = 0, VarType* vartype = nullptr, ParseFunction* func = nullptr);
	void ParseExprConvertToRPN(vector<SymbolNode>& exit, vector<SymbolNode>& stack, VarType* vartype, ParseFunction* func);
	BASIC_SYMBOL ParseExprPart(vector<SymbolNode>& exit, vector<SymbolNode>& stack, VarType* vartype, ParseFunction* func);
	void ParseExprPartPost(BASIC_SYMBOL& symbol, vector<SymbolNode>& exit, vector<SymbolNode>& stack);
	void ParseExprApplySymbol(vector<ParseNode*>& stack, SymbolNode& sn);
	ParseNode* ParseAssign(SymbolInfo& si, NodeRef& left, NodeRef& right);
	ParseNode* ParseConstExpr();
	void ParseArgs(vector<ParseNode*>& nodes, char open = '(', char close = ')');
	ParseNode* ParseItem(VarType* vartype = nullptr, ParseFunction* func = nullptr);
	ParseNode* ParseConstItem();

	void CheckFindItem(const string& id, bool is_func);
	ParseVar* GetVar(ParseNode* node);
	VarType GetVarType(bool is_arg, bool in_cpp = false, Type* type = nullptr);
	VarType GetVarTypeForMember();
	void PushSymbol(SYMBOL symbol, vector<SymbolNode>& exit, vector<SymbolNode>& stack, ParseNode* node = nullptr);
	bool GetNextSymbol(BASIC_SYMBOL& symbol);
	BASIC_SYMBOL GetSymbol(bool full_over = false);
	OpResult CanOp(SYMBOL symbol, SYMBOL real_symbol, ParseNode* lnode, ParseNode* rnode);
	bool TryConstExpr(ParseNode* left, ParseNode* right, ParseNode* op, SYMBOL symbol);
	bool TryConstExpr1(ParseNode* node, SYMBOL symbol);

	bool Cast(ParseNode*& node, VarType vartype, int cast_flags = 0, CastResult* cast_result = nullptr);
	bool TryCast(ParseNode*& node, VarType vartype, bool implici = true, bool pass_by_ref = false);
	bool DoConstCast(ParseNode* node, VarType vartype);
	CastResult MayCast(ParseNode* node, VarType vartype, bool pass_by_ref);
	void ForceCast(ParseNode*& node, VarType vartype, cstring op);
	bool TryConstCast(ParseNode*& node, VarType type);
	bool CanTakeRef(ParseNode* node, bool allow_ref = true);
	Op PushToSet(ParseNode* node);

	void Optimize();
	ParseNode* OptimizeTree(ParseNode* node);

	void CheckReturnValues();
	void CheckGlobalReturnValue();
	void VerifyFunctionReturnValue(ParseFunction* f);
	RETURN_INFO VerifyNodeReturnValue(ParseNode* node, bool in_switch);

	void CopyFunctionChangedStructs();
	void ConvertToBytecode();
	void ToCode(vector<int>& code, ParseNode* node, vector<uint>* break_pos);

	VarType GetReturnType(ParseNode* node);
	cstring GetName(ParseVar* var);
	cstring GetName(VarType vartype, int generic_type = V_VOID);
	cstring GetTypeName(ParseNode* node);
	VarType CommonType(VarType a, VarType b);
	FOUND FindItem(const string& id, Found& found);
	Function* FindFunction(const string& name);
	AnyFunction FindFunction(Type* type, const string& name);
	void FindAllFunctionOverloads(const string& name, vector<AnyFunction>& items);
	void FindAllFunctionOverloads(Type* type, const string& name, vector<AnyFunction>& funcs);
	void FindAllStaticFunctionOverloads(Type* type, const string& name, vector<AnyFunction>& funcs);
	AnyFunction FindEqualFunction(ParseFunction* pf);
	void FindAllCtors(Type* type, vector<AnyFunction>& funcs);
	AnyFunction FindFunction(Type* type, cstring name, delegate<bool(AnyFunction& f)> pred);
	AnyFunction FindSpecialFunction(Type* type, SpecialFunction spec, delegate<bool(AnyFunction& f)> pred);
	int MatchFunctionCall(ParseNode* node, CommonFunction& f, bool is_parse, bool obj_call);
	AnyFunction ApplyFunctionCall(ParseNode* node, vector<AnyFunction>& funcs, Type* type, bool ctor, bool obj_call = false);
	void CheckFunctionIsDeleted(CommonFunction& cf);
	bool CanOverload(BASIC_SYMBOL symbol);
	bool FindMatchingOverload(CommonFunction& f, BASIC_SYMBOL symbol);
	int GetNextType(); // 0-var, 1-ctor, 2-func, 3-operator, 4-type

	void AnalyzeCode();
	void AnalyzeType(Type* type);
	ParseFunction* AnalyzeArgs(VarType result, SpecialFunction special, Type* type, cstring name, int flags);
	VarType AnalyzeVarType();
	Type* AnalyzeAddType(const string& name);
	void AnalyzeMakeType(VarType& vartype, const string& name);
	void SetParseNodeFromMember(ParseNode* node, Member* m);
	bool HasSideEffects(ParseNode* node);
	void AnalyzeArgsDefaultValues(ParseFunction* f);
	
	Tokenizer t;
	Module* module;
	RunModule* run_module;
	vector<Str*> strs;
	vector<ParseFunction*> ufuncs;
	vector<ReturnInfo> global_returns;
	vector<int> complex_chain;
	Block* main_block, *current_block;
	ParseFunction* current_function;
	Type* current_type;
	ParseNode* global_node;
	int breakable_block, empty_string;
	CoreVarType global_result;
	bool optimize;
	vector<ReturnStructVar*> rsvs;
	uint prev_line;
	Enum* active_enum;
};
