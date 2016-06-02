#include <Cas.h>
#include <string>
#include <fstream>
#include <conio.h>
#include <iostream>

using namespace std;

cstring filename = "test.txt";

int main(int argc, char** argv)
{
	string path;
	if(argc == 2)
	{
		if(strcmp(argv[1], "-d") == 0)
			path = Format("../cases/%s", filename);
		else
			path = argv[1];
	}
	else
	{
		cout << "Usage: runner.exe file";
		return 0;
	}

	std::ifstream ifs(path);
	if(!ifs.is_open())
	{
		cout << Format("Failed to open file '%s'.", path.c_str());
		_getch();
		return 2;
	}
	std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	ifs.close();

	bool result = ParseAndRun(content.c_str());

	if(!result)
	{
		_getch();
		return 1;
	}

	return 0;
}
