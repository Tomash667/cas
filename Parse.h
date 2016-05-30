#pragma once

struct ParseContext
{
	string input;
	vector<string> strs;
	vector<int> code;
	uint vars;
};

bool Parse(ParseContext& ctx);
