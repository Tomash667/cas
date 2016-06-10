#pragma once

struct ParseContext
{
	string input;
	vector<string> strs;
	vector<int> code;
	uint vars;
	bool optimize;
};

bool Parse(ParseContext& ctx);
void InitializeParser();
void CleanupParser();
void Decompile(ParseContext& ctx);
