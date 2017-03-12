#pragma once

#include "cas/IMember.h"
#include "VarSource.h"
#include "VarType.h"

struct Type;

// Class member
struct Member : public cas::IMember, public VarSource
{
	enum UsedMode
	{
		No,
		Used,
		UsedBeforeSet,
		Set
	};

	Type* type;
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

	cas::IType* GetClass() override;
	cas::IModule* GetModule() override;
	cstring GetName() override;
	uint GetOffset() override;
	cas::ComplexType GetType() override;
};
