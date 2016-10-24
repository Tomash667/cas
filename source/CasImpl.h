#pragma once

#include "cas/Cas.h"

using namespace cas;

struct Function;
struct Member;
struct RunModule;
struct Type;
class Module;
class Parser;

void InitLib(Module& module, Settings& settings);
void Decompile(RunModule& run_module);
void Run(RunModule& run_module, ReturnValue& retval);
void Event(EventType event_type, cstring msg);
