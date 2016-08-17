#pragma once

#include "Cas.h"

using namespace cas;

struct Function;
struct Member;
struct RunModule;
struct Type;
class Module;
class Parser;

void InitCoreLib(Module& module, std::istream* input, std::ostream* output, bool use_getch);
void Decompile(RunModule& run_module);
void Run(RunModule& run_module, ReturnValue& retval);
void Event(EventType event_type, cstring msg);
