#pragma once

struct CasException
{
	cstring exc;
	CasException(cstring exc) : exc(exc) {}
};
