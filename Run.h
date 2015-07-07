#pragma once

struct ScriptFunction
{
	uint offset, args, locals;
};

void run(byte* code, vector<Str*>& strs, vector<ScriptFunction>& func, uint entry_point);
