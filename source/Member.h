#pragma once

#include "VarSource.h"
#include "VarType.h"

// Class member
struct Member : public VarSource
{
	enum UsedMode
	{
		No,
		Used,
		UsedBeforeSet,
		Set
	};

	string name;
	VarType vartype;
	int offset;
	union
	{
		bool bvalue;
		char cvalue;
		int value;
		float fvalue;
	};
	UsedMode used;
	tokenizer::Pos pos;
	bool have_def_value;
};
