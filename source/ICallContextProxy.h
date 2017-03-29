#pragma once

#include "cas/ICallContext.h"

struct Class;
struct RefVar;

// Proxy for call context calls from outside
class ICallContextProxy : public cas::ICallContext
{
public:
	virtual void AddAssert(cstring msg) = 0;
	virtual void GetGlobalPointer(int index, cas::Value& val) = 0;
	virtual void GetGlobalValue(int index, cas::Value& val) = 0;
	virtual void ReleaseClass(Class* c) = 0;

	static ICallContextProxy* Current;
#ifdef CHECK_LEAKS
	vector<Class*> all_classes;
	vector<RefVar*> all_refs;
#endif
};
