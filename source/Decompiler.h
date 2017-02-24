#pragma once

#include "cas/Settings.h"

class Module;

class Decompiler
{
public:
	void Init(cas::Settings& settings);
	void Decompile(Module& module);

	static Decompiler& Get()
	{
		static Decompiler decompiler;
		return decompiler;
	}

private:
	Decompiler();
	void DecompileType(int type, int val);
	void GetNextFunction(int* pos);

	Module* module;
	std::ostream* s_output;
	int current_function; //-2 not checked, -1 main
	bool decompile_marker;
};
