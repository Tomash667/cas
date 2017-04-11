#pragma once

#include "Function.h"

class Module;
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
typedef ObjectPoolVectorRef<ParseNode> NodeVectorRef;

struct ParseSettings
{
	string input;
	bool optimize;
	bool disallow_global_code;
	bool disallow_globals;
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

enum NextType
{
	NT_FUNC,
	NT_CALL,
	NT_VAR_DECL,
	NT_INVALID
};

class Parser
{
public:
	Parser(Module& module);
	~Parser();

	bool VerifyTypeName(cstring type_name, int& type_index);
	CodeFunction* ParseFuncDecl(cstring decl, Type* type, bool builtin);
	Member* ParseMemberDecl(cstring decl);
	Global* ParseGlobalDecl(cstring decl);
	bool GetFunctionNameDecl(cstring decl, string* name, string* real_decl, Type* type);
	bool Parse(ParseSettings& settigns);
	int CreateDefaultFunctions(Type* type, int define_ctor = -1);
	bool CheckTypeLoop(Type* type);
	void Reset();

private:
	void ConvertParseToScriptItems();
	void AddKeywords();
	void Cleanup();

	void ParseCode();
	ParseNode* ParseLineOrBlock();
	ParseNode* ParseBlock(ParseFunction* f = nullptr);
	ParseNode* ParseLine();
	ParseNode* ParseIf();
	ParseNode* ParseDo();
	ParseNode* ParseWhile();
	ParseNode* ParseFor();
	ParseNode* ParseBreak();
	ParseNode* ParseReturn();
	void ParseClass(bool is_struct);
	ParseNode* ParseSwitch();
	ParseNode* ParseCase(ParseNode* swi);
	void ParseEnum(bool forward);
	void ParseMemberDeclClass(Type* type, uint& pad);
	ParseNode* ParseFunc();
	void ParseFuncModifiers(bool have_type, int& flags);
	void ParseFuncInfo(Function* f, Type* type, bool in_cpp);
	void ParseFunctionArgs(Function* f, bool in_cpp);
	ParseNode* ParseVarTypeDecl();
	ParseNode* ParseCond();
	ParseNode* GetDefaultValueForVarDecl(VarType type);
	ParseNode* ParseVarCtor(VarType vartype);
	ParseNode* ParseVarDecl(VarType vartype);
	ParseNode* ParseExpr(ParseFunction* func = nullptr);
	void ParseExprConvertToRPN(vector<SymbolNode>& exit, vector<SymbolNode>& stack, ParseFunction* func);
	BASIC_SYMBOL ParseExprPart(vector<SymbolNode>& exit, vector<SymbolNode>& stack, ParseFunction* func);
	void ParseExprPartPost(BASIC_SYMBOL& symbol, vector<SymbolNode>& exit, vector<SymbolNode>& stack);
	void ParseExprApplySymbol(vector<ParseNode*>& stack, SymbolNode& sn);
	ParseNode* ParseAssign(SymbolInfo& si, NodeRef& left, NodeRef& right);
	ParseNode* ParseConstExpr();
	void ParseArgs(vector<ParseNode*>& nodes, char open = '(', char close = ')');
	ParseNode* ParseItem(ParseFunction* func = nullptr);
	ParseNode* ParseConstItem();
	bool IsConstExpr(ParseNode* node);

	void CheckFindItem(const string& id, bool is_func);
	ParseVar* GetVar(ParseNode* node);
	VarType GetVarType(bool is_arg, bool in_cpp = false, bool optional = false);
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
	Op GetFunctionOp(AnyFunction& f, bool is_ctor);

	void Optimize();
	ParseNode* OptimizeTree(ParseNode* node);

	void CheckReturnValues();
	void VerifyFunctionReturnValue(ParseFunction* f);
	RETURN_INFO VerifyNodeReturnValue(ParseNode* node, bool in_switch);

	void CopyFunctionChangedStructs();
	void ConvertToBytecode();
	void ToCode(vector<int>& code, ParseNode* node, vector<uint>* break_pos);

	Type* GetType(int index);
	Type* FindType(const string& name);
	cstring GetName(ParseVar* var);
	cstring GetTypeName(ParseNode* node);
	VarType CommonType(VarType a, VarType b);
	FOUND FindItem(const string& id, Found& found);
	void FindAllFunctionOverloads(const string& name, vector<AnyFunction>& items);
	AnyFunction FindEqualFunction(ParseFunction* pf);
	VarType GetRequiredType(ParseNode* node, Arg& arg);
	int MatchFunctionCall(ParseNode* node, Function& f, bool is_parse, bool obj_call);
	AnyFunction ApplyFunctionCall(ParseNode* node, vector<AnyFunction>& funcs, Type* type, bool ctor, bool obj_call = false);
	void CheckFunctionIsDeleted(Function& cf);
	bool CanOverload(BASIC_SYMBOL symbol);
	bool FindMatchingOverload(Function& f, BASIC_SYMBOL symbol);
	NextType GetNextType(bool analyze);
	void FreeTmpStr(string* str);
	bool IsCtor(ParseNode* node);
	void CheckGlobalCodeDisallowed();

	void AnalyzeCode();
	void AnalyzeClass();
	void AnalyzeLine(Type* type);
	ParseFunction* AnalyzeArgs(VarType result, SpecialFunction special, Type* type, cstring name, int flags);
	VarType AnalyzeVarType();
	Type* AnalyzeAddType(const string& name);
	void AnalyzeMakeType(VarType& vartype, const string& name);
	void SetParseNodeFromMember(ParseNode* node, Member* m);
	bool HasSideEffects(ParseNode* node);
	void VerifyAllTypes();
	void VerifyFunctionsDefaultArguments();
	void AnalyzeArgsDefaultValues(ParseFunction* pf);
	
	Tokenizer t;
	Module& module;
	vector<ParseFunction*> parse_funcs;
	Block* main_block, *current_block;
	ParseFunction* current_function;
	Type* current_type;
	ParseNode* global_node;
	Enum* active_enum;
	vector<ReturnStructVar*> rsvs;
	vector<string*> tmp_strs;
	uint prev_line, new_types_offset, new_globals_offset, parse_func_offset;
	int breakable_block;
	bool optimize, disallow_global_code, disallow_globals;
};
