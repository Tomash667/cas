#pragma once

#include "cas/Cas.h"

using namespace cas;

struct Function;
struct Member;
struct Type;
class Module;
class Parser;

void InitLib(Module& module, Settings& settings);
bool Run(Module& module);
void Event(EventType event_type, cstring msg);

struct CasException
{
	cstring exc;
	CasException(cstring exc) : exc(exc) {}
};

#include "Decompiler.h"
