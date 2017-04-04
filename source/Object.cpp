#include "Pch.h"
#include "Class.h"
#include "Global.h"
#include "ICallContextProxy.h"
#include "IModuleProxy.h"
#include "Object.h"
#include "Member.h"
#include "Type.h"

void Object::AddRef()
{
	if(is_clas)
		clas->refs++;
}

cas::Type Object::GetType()
{
	if(is_clas)
		return cas::Type(clas->type->GetGenericType(), clas->type);
	else
		return global->GetType();
}

cas::Value Object::GetMemberValue(cas::IMember* member)
{
	assert(is_clas);
	assert(member);
	Member* m = (Member*)member;
	assert(m->type == clas->type);
	cas::Value v;
	v.type = m->GetType();
	clas->copy_data(m->offset, clas->type->module_proxy->GetType(m->vartype.type)->size, v.int_value);
	return v;
}

cas::Value Object::GetMemberValueRef(cas::IMember* member)
{
	assert(is_clas);
	assert(member);
	Member* m = (Member*)member;
	assert(m->type == clas->type);
	cas::Value v;
	v.type = m->GetType();
	v.type.is_ref = true;
	v.int_value = (int)clas->data(m->offset);
	return v;
}

void* Object::GetPtr()
{
	assert(is_clas);
	return (void*)clas->data();
}

cas::Value Object::GetValue()
{
	assert(!is_clas);
	cas::Value v;
	v.type = global->GetType();
	if(global->ptr)
	{
		switch(global->vartype.type)
		{
		case V_BOOL:
			v.bool_value = *(bool*)global->ptr;
			break;
		case V_CHAR:
			v.char_value = *(char*)global->ptr;
			break;
		case V_INT:
			v.int_value = *(int*)global->ptr;
			break;
		case V_FLOAT:
			v.float_value = *(float*)global->ptr;
			break;
		default:
			assert(0);
			break;
		}
	}
	else
		context->GetGlobalValue(global->index, v);
	return v;
}

cas::Value Object::GetValueRef()
{
	assert(!is_clas);
	cas::Value v;
	v.type = global->GetType();
	v.type.is_ref = true;
	if(global->ptr)
		v.int_value = (int)global->ptr;
	else
		context->GetGlobalPointer(global->index, v);
	return v;
}

bool Object::IsGlobal()
{
	return !is_clas;
}

void Object::Release()
{
	if(is_clas)
		clas->Release();
}
