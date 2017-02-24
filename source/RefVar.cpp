#include "Pch.h"
#include "Class.h"
#include "RefVar.h"
#include "Str.h"

#ifdef CHECK_LEAKS
vector<RefVar*> RefVar::all_refs;
#endif

// hopefully no one will use function with 999 args
RefVar::RefVar(Type type, uint index, int var_index, uint depth) : type(type), refs(1), index(index), var_index(var_index), depth(depth), is_valid(true),
	to_release(false), ref_to_class(false)
{
#ifdef CHECK_LEAKS
	all_refs.push_back(this);
#endif
}

RefVar::~RefVar()
{
	if(type == MEMBER || ref_to_class)
		clas->Release();
	else if(type == INDEX)
		str->Release();
	else if(type == CODE && to_release)
		clas->Release();
}

bool RefVar::Release()
{
	assert(refs >= 1);
	if(--refs == 0)
	{
#ifdef CHECK_LEAKS
		RemoveElement(all_refs, this);
#endif
		delete this;
		return true;
	}
	else
		return false;
}
