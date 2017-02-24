#pragma once

// Operation variable source
// Used by parser to keep track on operations on variables/members
struct VarSource
{
	int index;
	bool mod, is_code_class;
};
