#pragma once

#include "cas/ICallContext.h"

struct Class;

// Proxy for call context calls from outside
class ICallContextProxy : public cas::ICallContext
{
public:
	virtual void AddAssert(cstring msg) = 0;
	virtual void ReleaseClass(Class* c) = 0;

	static ICallContextProxy* Current;
};
