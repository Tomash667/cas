#pragma once

// Pooled string implementation
struct Str : ObjectPoolProxy<Str>
{
	string s;
	int refs;

	bool Release()
	{
		--refs;
		if(refs == 0)
		{
			Free();
			return true;
		}
		else
			return false;
	}

	static Str* Create()
	{
		Str* str = Str::Get();
		str->refs = 1;
		return str;
	}

	static Str* Create(cstring s)
	{
		Str* str = Str::Get();
		str->s = s;
		str->refs = 1;
		return str;
	}
};
