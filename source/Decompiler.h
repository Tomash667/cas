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

	Module* module;
	std::ostream* s_output;
	bool decompile_marker;
};
