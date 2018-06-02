/* help.cpp
 * *HELP command handler
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>			// Included for printf()
#include "star_help.h"
#include "../main.h"			// Included for STARTUP_MESSAGE
#include "../commands/commands.h"	// Included for lsh_num_builtins, builtin_cmd and builtin_help

using namespace std;



namespace commands {
	int help(char **args) {
		int i;

		printf("%s v%s.%s.%s\n\n", STARTUP_MESSAGE, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCHLEVEL);
		for (i = 0; i < (totalNumOfCommands()); i++) {
			printf("   %s%s\n", cmds[i][0], cmds[i][1]);
		}
		printf("\n");

		return(0);
	}
}


