#pragma once

#include "cas/IObject.h"

struct Class;

struct Object final : public cas::IObject
{
	void AddRef() override;
	cas::Type GetType() override;
	cas::Value GetMemberValue(cas::IMember* member) override;
	cas::Value GetMemberValueRef(cas::IMember* member) override;
	void* GetPtr() override;
	void Release() override;

	Class* clas;
};
