#pragma once

struct ParseContext
{
	string input;
	vector<string> strs;
	vector<int> code;
	vector<UserFunction> ufuncs;
	uint globals, entry_point;
	bool optimize;
};

bool Parse(ParseContext& ctx);
void InitializeParser();
void CleanupParser();
void Decompile(ParseContext& ctx);
