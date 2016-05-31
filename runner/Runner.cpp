#include <Cas.h>
#include <string>
#include <fstream>
#include <conio.h>
#include <iostream>

using namespace std;

cstring filename = "const_math.txt";

int main()
{
	string path(Format("../cases/%s", filename));
	std::ifstream ifs(path);
	if(!ifs.is_open())
	{
		cout << Format("Failed to open file '%s'.", path.c_str());
		_getch();
		return 2;
	}
	std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

	bool result = ParseAndRun(content.c_str());

	if(!result)
	{
		_getch();
		return 1;
	}

	return 0;
}
