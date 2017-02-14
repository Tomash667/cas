#pragma once

class Decompiler
{
public:
	void Init(Settings& settings);
	void Decompile(Module& module);

	inline static Decompiler& Get()
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
