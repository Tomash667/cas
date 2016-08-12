#pragma once

#include "Cas.h"

using namespace cas;

struct Function;
struct Member;
class Module;
class Parser;
struct Type;

extern EventHandler handler;

// CoreLib
void InitCoreLib(Module* module, std::istream* input, std::ostream* output, bool use_getch);
