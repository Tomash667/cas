#pragma once

typedef unsigned char byte;
typedef unsigned int uint;
typedef const char* cstring;

#define BIT(bit) (1<<(bit))
#define IS_SET(flaga,bit) (((flaga) & (bit)) != 0)
#define IS_CLEAR(flaga,bit) (((flaga) & (bit)) == 0)
#define IS_ALL_SET(flaga,bity) (((flaga) & (bity)) == (bity))
#define SET_BIT(flaga,bit) ((flaga) |= (bit))
#define CLEAR_BIT(flaga,bit) ((flaga) &= ~(bit))
#define SET_BIT_VALUE(flaga,bit,wartos) { if(wartos) SET_BIT(flaga,bit); else CLEAR_BIT(flaga,bit); }
#define COPY_BIT(flaga,flaga2,bit) { if(((flaga2) & (bit)) != 0) SET_BIT(flaga,bit); else CLEAR_BIT(flaga,bit); }

#include <string>
#include <vector>
#include <cassert>
#include <cstdarg>

using std::vector;
using std::string;

extern string g_tmp_string, g_tmp_string2;

template<typename T>
inline T& Add1(vector<T>& v)
{
	v.resize(v.size() + 1);
	return v.back();
}

cstring Format(cstring fmt, ...);
bool LoadFileToString(cstring path, string& str);
int StringToNumber(cstring s, __int64& i, float& f);
void Unescape(const string& sin, string& sout);
