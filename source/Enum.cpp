#include "Pch.h"
#include "Enum.h"

std::pair<string, int>* Enum::Find(const string& id)
{
	for(auto& val : values)
	{
		if(val.first == id)
			return &val;
	}
	return nullptr;
}
