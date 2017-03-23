#pragma once

#include "cas/IModule.h"
#include "VarType.h"

struct Type;

// Proxy for module calls from outside
class IModuleProxy : public cas::IModule
{
public:
	virtual bool AddEnumValue(Type* type, cstring name, int value) = 0;
	virtual bool AddMember(Type* type, cstring decl, int offset) = 0;
	virtual	bool AddMethod(Type* type, cstring decl, const cas::FunctionInfo& func_info) = 0;
	virtual bool GetFunctionDecl(cstring decl, string& real_decl, Type* type) = 0;
	virtual cstring GetName(VarType vartype) = 0;
	virtual Type* GetType(int index) = 0;
	virtual cas::Type VarTypeToType(VarType vartype) = 0;
};
