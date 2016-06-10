#include <Cas.h>
#include <string>
#include <fstream>
#include <conio.h>
#include <iostream>

using namespace std;

cstring def_filename = "while.txt";
const bool def_optimize = true;
const bool def_decompile = false;

int main(int argc, char** argv)
{
	string path;
	bool optimize = def_optimize;
	bool decompile = def_decompile;
	bool str = false;

	for(int i = 1; i < argc; ++i)
	{
		cstring arg = argv[i];
		if(arg[0] != '-')
		{
			path = arg;
			str = false;
		}
		else if(strcmp(arg, "-h") == 0 || strcmp(arg, "-help") == 0 || strcmp(arg, "-?") == 0)
		{
			cout << "CaScript runner, command:\n"
				"-h/help/? - show help\n"
				"-debug - use builtin test case\n"
				"-decomp - decompile bytecode\n"
				"-nodecomp - don't decompile bytecode\n"
				"-op - optimize bytecode\n"
				"-noop - don't optimize bytecode\n"
				"-str - use code from string in next parameter\n"
				"Other paramters are used as filename to run. Only last path is used.\n";
		}
		else if(strcmp(arg, "-debug") == 0)
		{
			path = Format("../cases/%s", def_filename);
			str = false;
		}
		else if(strcmp(arg, "-decomp") == 0)
			decompile = true;
		else if(strcmp(arg, "-nodecomp") == 0)
			decompile = false;
		else if(strcmp(arg, "-op") == 0)
			optimize = true;
		else if(strcmp(arg, "-noop") == 0)
			optimize = false;
		else if(strcmp(arg, "-str") == 0)
		{
			++i;
			if(i < argc)
			{
				path = argv[i];
				str = true;
			}
			else
				cout << "ERROR: Missing string input.\n";
		}
	}

	if(path.empty())
	{
		cout << "Missing input file. Use runner.exe -? for help.\n";
		return 0;
	}

	string content;
	if(str)
		content = path;
	else
	{
		ifstream ifs(path);
		if(!ifs.is_open())
		{
			cout << Format("Failed to open file '%s'.", path.c_str());
			_getch();
			return 2;
		}
		content = string((istreambuf_iterator<char>(ifs)), (istreambuf_iterator<char>()));
		ifs.close();
	}

	bool result = ParseAndRun(content.c_str(), optimize, decompile);

	if(!result)
	{
		_getch();
		return 1;
	}

	return 0;
}
