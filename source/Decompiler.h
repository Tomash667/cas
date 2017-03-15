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
	void DecompileFunctions();
	void DecompileMain();
	void DecompileBlock(int* start, int* end);
	void DecompileType(int type, int val);

	Module* module;
	std::ostream* s_output;
	bool decompile_marker;
};
