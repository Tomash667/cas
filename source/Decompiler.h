#pragma once

class Decompiler
{
public:
	void Init(Settings& settings);
	void Decompile(RunModule& module);

	inline static Decompiler& Get()
	{
		static Decompiler decompiler;
		return decompiler;
	}

private:
	Decompiler();
	void DecompileType(int type, int val);

	Parser* parser;
	RunModule* module;
	std::ostream* s_output;
	bool decompile_marker;
};
