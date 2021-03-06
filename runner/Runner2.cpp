/*#include <cas/Cas.h>
#include <Core/Core.h>
#include <vld.h>
#include <string>
#include <fstream>
#include <conio.h>
#include <iostream>
#include <vector>

using namespace std;
using namespace cas;

cstring def_filename = "class_ref.txt";
const bool def_optimize = true;
const bool def_decompile = false;

bool decompile, optimize, standard_entry_point;

void HandleEvents(EventType event_type, cstring msg)
{
	cstring type;
	switch(event_type)
	{
	case EventType::Info:
		type = "INFO";
		break;
	case EventType::Warning:
		type = "WARN";
		break;
	case EventType::Error:
	default:
		type = "ERROR";
		break;
	}
	cout << type;
	cout << msg;
	cout << '\n';
}

class CommandLineParser
{
public:
	enum Cmd
	{
		C_NONE = -1,
		C_INVALID = -2
	};

	bool Parse();
	void AddSwitch(int id, cstring text);
	void AddSwitch(int id, std::initializer_list<cstring> const& texts);
	int GetCurrentCommandId();
	const string& GetCurrentCommandText();
	Tokenizer& GetTokenizer();
};

enum Cmd
{
	C_HELP,
	C_DEBUG,
	C_DECOMP,
	C_NODECOMP,
	C_OP,
	C_NOOP,
	C_STR,
	C_ENTRY
};*/

/*
"-h/help/? - show help\n"
"-debug - use builtin test case\n"
"-decomp - decompile bytecode\n"
"-nodecomp - don't decompile bytecode\n"
"-op - optimize bytecode\n"
"-noop - don't optimize bytecode\n"
"-str STR - use code from string in next parameter\n"
"-e/entry STR - set function entry point\n"
*/

/*void Run

int main(int argc, char** argv)
{
	SetHandler(HandleEvents);
	Settings settings;
	settings.use_debuglib = true;
	if(!Initialize(&settings))
	{
		cout << "Failed to initialize Cas.\n\n(OK)";
		_getch();
		return 3;
	}

	bool optimize = def_optimize;
	bool decompile = def_decompile;
	std::vector<std::pair<string, bool>> input;

	CommandLineParser parser;
	parser.AddSwitch(C_HELP, { "h", "help", "?" });
	parser.AddSwitch(C_DEBUG, "debug");
	parser.AddSwitch(C_DECOMP, "decomp");
	parser.AddSwitch(C_NODECOMP, "nodecomp");
	parser.AddSwitch(C_OP, "op");
	parser.AddSwitch(C_NOOP, "noop");
	parser.AddSwitch(C_STR, "str");
	parser.AddSwitch(C_ENTRY, { "e", "entry" });

	while(parser.Parse())
	{
		switch(parser.GetCommandId())
		{
		case CommandLineParser::C_INVALID:
			cout << "ERROR: Unknown switch '" << parser.GetCurrentCommandText() << "'.\n";
			break;
		case CommandLineParser::C_NONE:
			break;
		case C_HELP:
			cout << "CaScript runner, command:\n"
				"-h/help/? - show help\n"
				"-debug - use builtin test case\n"
				"-decomp - decompile bytecode\n"
				"-nodecomp - don't decompile bytecode\n"
				"-op - optimize bytecode\n"
				"-noop - don't optimize bytecode\n"
				"-str STR - use code from string in next parameter\n"
				"-e/entry STR - set function entry point (STR can be function name or declaration)\n"
				"-defentry - use default entry point (main or global)\n"
				"Other paramters are used as filename to run.\n";
			break;
		case C_DEBUG:
		case C_DECOMP:
			decompile = true;
			break;
		case C_NODECOMP:
			decompile = false;
			break;
		case C_OP:
			optimize = true;
			break;
		case C_NOOP:
			optimize = false;
			break;
		case C_STR:
		case C_ENTRY:
			break;
		}
	}



	for(int i = 1; i < argc; ++i)
	{
		cstring arg = argv[i];
		if(arg[0] != '-')
			input.push_back(std::pair<string, bool>(arg, false));
		else if(strcmp(arg, "-h") == 0 || strcmp(arg, "-help") == 0 || strcmp(arg, "-?") == 0)
		{

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
		else if(strcmp(arg, "-e") == 0 || strcmp(arg, "-entry") == 0)
		{

		}
		else
			cout << "ERROR: Unknown switch '" << arg << "'.\n";
	}

	if(input.empty())
	{
		cout << "Missing input file. Use runner.exe -? for help.\n\n(OK)";
		_getch();
		return 0;
	}

	IModule* module = CreateModule();

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
		vector<string>& asserts = GetAsserts();
		if(!result)
		{
			cout << "\n\n(OK)";
			_getch();
		}
		else if(!asserts.empty())
		{
			cout << Format("\n\nAsserts failed (%u). ", asserts.size());
			for(string& s : asserts)
			{
				cout << s;
				cout << " ";
			}
			cout << "\n\n(ok)";
			_getch();
		}
		asserts.clear();
	}

	Shutdown();
	return 0;
}
*/