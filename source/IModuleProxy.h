#pragma once

#include "cas/FunctionInfo.h"

struct Type;

// Proxy for module calls from outside
class IModuleProxy
{
public:
	virtual bool AddEnumValue(Type* type, cstring name, int value) = 0;
	virtual bool AddMember(Type* type, cstring decl, int offset) = 0;
	virtual	bool AddMethod(Type* type, cstring decl, const cas::FunctionInfo& func_info) = 0;
};
