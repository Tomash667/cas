#pragma once

// Call context stack frame
struct StackFrame
{
	enum Type
	{
		NORMAL,
		CTOR,
		DTOR
	};

	uint expected_stack, pos;
	int current_line, current_function;
	Type type;
};
