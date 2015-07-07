#include "Base.h"
#include <windows.h>

string g_tmp_string, g_tmp_string2;
DWORD tmp;

cstring Format(cstring str, ...)
{
	const uint FORMAT_STRINGS = 8;
	const uint FORMAT_LENGTH = 2048;

	assert(str);

	static char buf[FORMAT_STRINGS][FORMAT_LENGTH];
	static int marker = 0;

	va_list list;
	va_start(list, str);
	_vsnprintf_s((char*)buf[marker], FORMAT_LENGTH, FORMAT_LENGTH - 1, str, list);
	char* cbuf = buf[marker];
	cbuf[FORMAT_LENGTH - 1] = 0;

	marker = (marker + 1) % FORMAT_STRINGS;

	return cbuf;
}

bool LoadFileToString(cstring path, string& str)
{
	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
		return false;

	DWORD size = GetFileSize(file, NULL);
	str.resize(size);

	ReadFile(file, (char*)str.c_str(), size, &tmp, NULL);

	CloseHandle(file);

	return size == tmp;
}

int StringToNumber(cstring s, __int64& i, float& f)
{
	assert(s);

	i = 0;
	f = 0;
	uint diver = 10;
	uint digits = 0;
	char c;
	bool sign = false;
	if(*s == '-')
	{
		sign = true;
		++s;
	}

	while((c = *s) != 0)
	{
		if(c == '.')
		{
			++s;
			break;
		}
		else if(c >= '0' && c <= '9')
		{
			i *= 10;
			i += (int)c - '0';
		}
		else
			return 0;
		++s;
	}

	if(c == 0)
	{
		if(sign)
			i = -i;
		f = (float)i;
		return 1;
	}

	while((c = *s) != 0)
	{
		if(c == 'f')
		{
			if(digits == 0)
				return 0;
			break;
		}
		else if(c >= '0' && c <= '9')
		{
			++digits;
			f += ((float)((int)c - '0')) / diver;
			diver *= 10;
		}
		else
			return 0;
		++s;
	}
	f += (float)i;
	if(sign)
	{
		f = -f;
		i = -i;
	}
	return 2;
}

int strchr_index(cstring chrs, char c)
{
	int index = 0;
	do
	{
		if(*chrs == c)
			return index;
		++index;
		++chrs;
	} while(*chrs);

	return -1;
}

void Unescape(const string& sin, string& sout)
{
	sout.clear();
	sout.reserve(sin.length());

	cstring unesc = "nt\\\"";
	cstring esc = "\n\t\\\"";

	for(uint i = 0, len = sin.length(); i<len; ++i)
	{
		if(sin[i] == '\\')
		{
			++i;
			if(i == len)
			{
				//ERROR(Format("Unescape error in string \"%s\", character '\\' at end of string.", sin.c_str()));
				break;
			}
			int index = strchr_index(unesc, sin[i]);
			if(index != -1)
				sout += esc[index];
			else
			{
				//ERROR(Format("Unescape error in string \"%s\", unknown escape sequence '\\%c'.", sin.c_str(), sin[i]));
				break;
			}
		}
		else
			sout += sin[i];
	}
}
