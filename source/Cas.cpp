#include "Pch.h"
#include "Function.h"
#include "CasImpl.h"
#include "Type.h"
#include "Module.h"

using namespace cas;

Logger* logger;
static Module* core_module;
static int module_index;
static bool initialized;
static bool have_errors;
static bool using_event_logger;






