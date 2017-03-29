#pragma once

#include "cas/IObject.h"

struct Class;
struct Global;
class ICallContextProxy;

struct Object final : public cas::IObject
{
	void AddRef() override;
	cas::Type GetType() override;
	cas::Value GetMemberValue(cas::IMember* member) override;
	cas::Value GetMemberValueRef(cas::IMember* member) override;
	void* GetPtr() override;
	cas::Value GetValue() override;
	cas::Value GetValueRef() override;
	bool IsGlobal() override;
	void Release() override;

	ICallContextProxy* context;
	union
	{
		Class* clas;
		Global* global;
	};
	bool is_clas;
};
