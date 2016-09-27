#include "Pch.h"
#include "TestEnvironment.h"
#include <conio.h>

int main(int ac, char* av[])
{
	TestEnvironment* env = new TestEnvironment;

	// command line
	for(int i = 1; i < ac; ++i)
	{
		if(strcmp(av[i], "-decompile") == 0)
			env->decompile = true;
	}

	testing::InitGoogleTest(&ac, av);
	testing::AddGlobalTestEnvironment(env);
	int result = RUN_ALL_TESTS();
	if(!TestEnvironment::CI_MODE)
	{
		printf("\n\nPress any key to continue...");
		_getch();
	}
	//VLDDisable();
	return result;
}
