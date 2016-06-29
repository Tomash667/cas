#pragma once

struct ParseContext
{
	string input;
	vector<Str*> strs;
	vector<int> code;
	vector<UserFunction> ufuncs;
	uint globals, entry_point;
	bool optimize;
};

bool Parse(ParseContext& ctx);
void InitializeParser();
void CleanupParser();
void Decompile(ParseContext& ctx);
Function* ParseFuncDecl(cstring decl);
Member* ParseMember(cstring decl);
void AddParserType(Type* type);
