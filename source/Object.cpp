#include "Pch.h"
#include "Class.h"
#include "IModuleProxy.h"
#include "Object.h"
#include "Member.h"
#include "Type.h"

void Object::AddRef()
{
	clas->refs++;
}

cas::Type Object::GetType()
{
	return cas::Type(cas::GenericType::Object, clas->type);
}

cas::Value Object::GetMemberValue(cas::IMember* member)
{
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
	return (void*)clas->data();
}

void Object::Release()
{
	clas->Release();
}
