#pragma once

using namespace cas;

const bool CI_MODE = ((_CI_MODE - 1) == 0);

extern IModule* def_module;

void RunFileTest(IModule* module, cstring filename, cstring input, cstring output, bool optimize = true);
inline void RunFileTest(cstring filename, cstring input, cstring output, bool optimize = true)
{
	RunFileTest(def_module, filename, input, output, optimize);
}

void RunTest(IModule* module, cstring code);
inline void RunTest(cstring code)
{
	RunTest(def_module, code);
}

void RunFailureTest(IModule* module, cstring code, cstring error);
inline void RunFailureTest(cstring code, cstring error)
{
	RunFailureTest(def_module, code, error);
}

