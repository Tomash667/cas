#pragma once

struct CasException
{
	cstring exc;
	CasException(cstring exc) : exc(exc) {}
	
	template<typename... Args>
	CasException(cstring exc, const Args&... args) : exc(Format(exc, args...))
	{
	}
};
