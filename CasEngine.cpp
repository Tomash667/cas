/*#include <cstdio>
#include <vector>
#include <string>

using std::vector;
using std::string;

typedef const char* cstring;

namespace Cas
{
	class Function
	{
	public:
	};

	class Packet
	{
	public:
		string name;
		vector<Function*> functions;
	};

	class Engine
	{
	public:
		Engine()
		{
			global = new Packet;
		}
		void RegisterFunction(cstring decl, void* ptr)
		{
			global->RegisterFunction(decl, ptr);
		}

	private:
		Packet* global;
		vector<Packet*> packets;
	};
};

class CasEngine
{
public:
	void RegisterFunction(cstring decl, void* ptr);
};

void test()
{
	printf("test");
}

void a(CasEngine* cas)
{
	cas->RegisterFunction("void test()", test);
}
*/
