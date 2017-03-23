#include "Pch.h"
#include "Class.h"
#include "ICallContextProxy.h"
#include "Member.h"
#include "Str.h"
#include "Type.h"

#ifdef CHECK_LEAKS
vector<Class*> Class::all_classes;
#endif

Class* Class::Create(Type* type)
{
	assert(type);
	byte* data = new byte[type->size + sizeof(Class)];
	Class* c = (Class*)data;
	c->refs = 1;
	c->type = type;
	c->is_code = false;
	c->adr = ((int*)&c->adr) + 1;
	memset(c->adr, 0, type->size);
	if(type->have_complex_member && type->IsCode())
	{
		for(Member* m : type->members)
		{
			if(m->vartype.type == V_STRING)
			{
				string* str = &c->at<string>(m->offset);
				new(str) string;
			}
		}
	}
#ifdef CHECK_LEAKS
	all_classes.push_back(c);
	c->attached = true;
#endif
	return c;
}

Class* Class::CreateCode(Type* type, int* real_class)
{
	assert(type);
	Class* c = new Class;
	c->refs = 1;
	c->type = type;
	c->is_code = true;
	c->adr = real_class;
#ifdef CHECK_LEAKS
	all_classes.push_back(c);
	c->attached = true;
#endif
	return c;
}

Class* Class::Copy(Class* base)
{
	assert(base);
	Type* type = base->type;
	byte* data = new byte[type->size + sizeof(Class)];
	Class* c = (Class*)data;
	c->type = type;
	c->refs = 1;
	c->is_code = false;
	c->adr = ((int*)&c->adr) + 1;
	memcpy(c->adr, base->adr, type->size);
	if(type->have_complex_member)
	{
		for(Member* m : type->members)
		{
			if(m->vartype.type == V_STRING)
			{
				if(!type->IsCode())
				{
					Str* str = c->at<Str*>(m->offset);
					str->refs++;
				}
				else
				{
					string& old_str = base->at<string>(m->offset);
					string* str = &c->at<string>(m->offset);
					new(str) string(old_str);
				}

			}
		}
	}
#ifdef CHECK_LEAKS
	all_classes.push_back(c);
	c->attached = true;
#endif
	return c;
}

bool Class::Release()
{
	assert(refs >= 1);
	if(--refs == 0)
	{
#ifdef CHECK_LEAKS
		if(attached)
			RemoveElement(all_classes, this);
#endif
		Delete();
		return true;
	}
	else
		return false;
}

void Class::Delete()
{
	bool mem_free = false;
	if(type->dtor)
	{
		if(is_code && !type->IsStruct())
			mem_free = true;
		ICallContextProxy::Current->ReleaseClass(this);
	}
	if(is_code)
	{
		if(type->IsRefCounted())
			ICallContextProxy::Current->ReleaseClass(this);
		if(!mem_free)
			delete this;
	}
	else if(!mem_free)
	{
		if(type->have_complex_member)
		{
			for(Member* m : type->members)
			{
				if(m->vartype.type == V_STRING)
				{
					if(!type->IsCode())
					{
						Str* str = at<Str*>(m->offset);
						str->Release();
					}
					else
					{
						string& str = at<string>(m->offset);
						str.~basic_string();
					}
				}
			}
		}
		byte* data = (byte*)this;
		delete[] data;
	}
}

#ifdef CHECK_LEAKS
void Class::Deattach()
{
	attached = false;
	RemoveElement(all_classes, this);
}
#endif
