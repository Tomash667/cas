/*#include <vector>
#include <string>

using std::vector;
using std::string;

typedef unsigned char byte;

enum Op
{
	push_arg,
	push_local,
	pop
};

enum VarType
{
	VOID,
	CHAR,
	INT,
	FLOAT,
	STR,
	//REF,
	RETURN_ADDRESS
};

struct Str
{
	string s;
	int refs;
};

struct _StrPool
{
	vector<Str*> v;

	inline Str* Get()
	{
		Str* s;
		if(!v.empty())
		{
			s = v.back();
			v.pop_back();
		}
		else
			s = new Str;
		return s;
	}

	inline void Free(Str* s)
	{
		v.push_back(s);
	}
} StrPool;

union VarValue
{
	char c;
	int i;
	float f;
	Str* s;
};

struct Var
{
	VarValue v;
	VarType type;
};

vector<Var> stack;
int args_offset, args_count, locals_offset, locals_count;

void run(byte* code, vector<Str*> strs)
{
	byte* c = code;
	while(true)
	{
		Op op = (Op)*c;
		++c;
		switch(op)
		{
		case push_local:
			{
				byte* b
			}
			break;
		case push_arg:
		case pop:
		}
	}
}
*/