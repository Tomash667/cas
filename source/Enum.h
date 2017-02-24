#pragma once

struct Type;

// Registered enumerator
struct Enum
{
	Type* type;
	vector<std::pair<string, int>> values;

	std::pair<string, int>* Find(const string& id);
};
