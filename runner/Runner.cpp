#include <Cas.h>
#include <string>
#include <fstream>
#include <conio.h>
#include <iostream>
#include <vector>

using namespace std;

cstring def_filename = "class.txt";
const bool def_optimize = true;
const bool def_decompile = false;
static bool have_errors;

void HandleEvents(cas::EventType event_type, cstring msg)
{
	if(event_type == cas::Error)
		have_errors = true;
	cstring type;
	switch(event_type)
	{
	case cas::Info:
		type = "INFO";
		break;
	case cas::Warning:
		type = "WARN";
		break;
	case cas::Error:
	default:
		type = "ERROR";
		break;
	}
	cout << type;
	cout << msg;
	cout << '\n';
}

int main(int argc, char** argv)
{
	cas::SetHandler(HandleEvents);
	cas::Initialize();

	if(have_errors)
	{
		cout << "Failed to initialize Cas.\n\n(OK)";
		_getch();
		return 3;
	}

	bool optimize = def_optimize;
	bool decompile = def_decompile;
	std::vector<std::pair<string, bool>> input;

	for(int i = 1; i < argc; ++i)
	{
		cstring arg = argv[i];
		if(arg[0] != '-')
			input.push_back(std::pair<string, bool>(arg, false));
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
			string s(def_filename), part;
			unsigned pos = 0;
			while(pos != string::npos)
			{
				unsigned pos2 = s.find_first_of(';', pos);
				if(pos2 == string::npos)
					part = s.substr(pos);
				else
					part = s.substr(pos, pos2 - pos);
				if(!part.empty())
					input.push_back(std::pair<string, bool>(Format("../cases/%s", part.c_str()), false));
				if(pos2 == string::npos)
					pos = string::npos;
				else
					pos = pos2 + 1;
			}
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
				input.push_back(std::pair<string, bool>(arg, true));
			else
				cout << "ERROR: Missing string input.\n";
		}
	}

	if(input.empty())
	{
		cout << "Missing input file. Use runner.exe -? for help.\n\n(OK)";
		_getch();
		return 0;
	}

	cas::IModule* module = cas::CreateModule();

	bool first = true;
	string content;
	for(auto& item : input)
	{
		if(first)
			first = false;
		else
			cout << "\n--------------------------------------------------\n";

		if(item.second)
			content = item.first;
		else
		{
			ifstream ifs(item.first);
			if(!ifs.is_open())
			{
				cout << Format("Failed to open file '%s'.\n\n(OK)", item.first.c_str());
				_getch();
				continue;
			}
			content = string((istreambuf_iterator<char>(ifs)), (istreambuf_iterator<char>()));
			ifs.close();
		}

		bool result = module->ParseAndRun(content.c_str(), optimize, decompile);
		if(!result)
		{
			cout << "\n\n(OK)";
			_getch();
			continue;
		}
	}

	cas::Shutdown();
	return 0;
}
