/* main.cpp
 * Econet gateway server: An Econet file- and print server with transparent ethernet-gateway functionality
 *
 * (c) Eelco Huininga 2017-2019
 */

#include <cstdio>			// Included for NULL, printf(), FILE*, fopen(), fgets(), feof() and fclose()
#include <cstring>			// Included for memmove(), strtok(), strcmp() and strlen()
#include <atomic>			// Included for std::atomic
#include <thread>			// Included for std::thread
#include <unistd.h>			// geteuid()
#include <readline/readline.h>		// readline(), rl_bind_key()
#include <readline/history.h>		// add_history()

#include "main.h"			// Header file for this C++ module
#include "cli.h"			// cli::user_id
#include "config.h"
#include "errorhandler.h"		// Error handling functions
#include "econet.h"			// Included for pollEconet() thread
#include "aun.h"			// Included for pollAUN() thread
#include "cli.h"			// All * commands
#include "netfs.h"			// netfs::dismount()
#include "users.h"			// Included for users::loadUsers()
#include "stations.h"			// Included for users::loadStations()
#include "platforms/platform.h"		// All platform- and hardware-dependant functions

using namespace std;

std::atomic<bool>	bye;
bool			bootdone;
FILE			*fp_volume;
FILE			*fp_bootfile;
Disc			*discs[ECONET_MAX_DISCDRIVES];



int main(int argc, char** argv) {
	char	*command = (char *) malloc(MAX_COMMAND_LENGTH);
	char	**args = NULL;
	int	result, cmdstart;

	/* Print startup message */
//	printf("%s v%s.%s.%s\n\n", STARTUP_MESSAGE, FILESTORE_VERSION_MAJOR, FILESTORE_VERSION_MINOR, FILESTORE_VERSION_PATCHLEVEL);
	printf(STARTUP_MESSAGE " v" FILESTORE_VERSION_MAJOR "." FILESTORE_VERSION_MINOR "." FILESTORE_VERSION_PATCHLEVEL "\n\n");

	/* Handle command line parameters */
	if (argc > 1) {
		printf("%s Command line handler; not yet implemented\n", argv[0]);
	}

	/* Initialize signal handlers */
	initSignals();

	/* Initialize the RaspberryPi GPIO hardware */
	if ((result = api::initializeHardware()) < 0) {
		errorHandler(0xC3140000);
		exit(result);
	}

	/* Reset the Econet module */
	if ((result = api::resetHardware()) != 0) {
		errorHandler(0xC3140001);
		exit(result);
	}

	/* Load !Users (users and passwords) */
	if ((users::loadUsers()) != 0) {
		errorHandler(0x000000D6);
		exit(0x000000D6);
	}

	/* Load !Stations (Econet to/from AUN translation table) */
	if ((stations::loadStations()) != 0) {
		errorHandler(0x000000D6);
		exit(0x000000D6);
	}

	/* Spawn new thread for polling hardware and processing network data */
//	std::thread thread_econet_listener(econet::pollNetworkReceive);
	std::thread thread_ipv4_aun_Listener(aun::ipv4_aun_Listener);
#if (FILESTORE_WITHIPV6 == 1)
	std::thread thread_ipv6_aun_Listener(aun::ipv6_aun_Listener);
#endif
#if (FILESTORE_WITHOPENSSL == 1)
	std::thread thread_ipv4_dtls_Listener(aun::ipv4_dtls_Listener);
#if (FILESTORE_WITHIPV6 == 1)
//	std::thread thread_ipv6_dtls_Listener(aun::ipv6_dtls_Listener);
#endif
#endif
	/* Announce that a new Econet bridge is online on the network */
	econet::sendBridgeAnnounce();

	/* Scan the Econet for existing bridges and networks */
	econet::sendWhatNetBroadcast();

	/* Load !Boot */
	fp_bootfile = fopen(BOOTFILE, "r");
	if (fp_bootfile == NULL) {
		bootdone = true;
	} else {
		printf("- Execing %s:\n", BOOTFILE);
		bootdone = false;
	}

	/* Disable readline's filename completion */
	rl_attempted_completion_function = cli::command_completion;

	/* Set flags before entering main loop */
	bye = false;
	econet::netmon = false;

	/* Main loop */
	while (bye == false) {
		/* Check network status */
		if (api::networkState())
			errorHandler(0x000003A3);

		if (bootdone == false) {
			fgets(command, MAX_COMMAND_LENGTH, fp_bootfile);
			if (feof(fp_bootfile)) {
				fclose(fp_bootfile);
				bootdone = true;
			}
		} else {
			if ((command = readline(PROMPT)) == nullptr)
				printf("Warning: Could not read stdin.\n");
		}

		/* Strip leading *'s */
		cmdstart = 0;
		while (command[cmdstart] == PROMPT[0])
			cmdstart++;

		if (cmdstart != 0)
			memmove(command, (command + cmdstart), strlen(command));

		/* When processing the !Boot script, print the command currently being processed */
		if (bootdone == false)
			printf("%s%s", PROMPT, command);

		/* Only process non-empty commands */
		if (strlen(command) > 0) {
			/* Decrease *ENABLE timer */
			if (users::users[cli::user_id].enable_counter > 0)
				users::users[cli::user_id].enable_counter--;

			/* Add command to readline history */
			if (bootdone == true)
				add_history(command);

			/* Split command line into tokens */
			args = tokenizeCommandLine(command);

			/* Execute command */
			executeCommand(args);

			/* Free memory claimed by tokenizeCommandLine() */
			free(args);
		}

		/* Release memory */
		if (bootdone == true)
			free(command);
	}

	printf("\n");

	/* Wait for the threads to finish */
//	thread_econet_listener.join();
	thread_ipv4_aun_Listener.join();
#if (FILESTORE_WITHIPV6 == 1)
	thread_ipv6_aun_Listener.join();
#endif
#if (FILESTORE_WITHOPENSSL == 1)
	thread_ipv4_dtls_Listener.join();
#if (FILESTORE_WITHIPV6 == 1)
//	thread_ipv6_dtls_Listener.join();
#endif
#endif

	/* Dismount all open disc images */
//	netfs::dismount(NULL);

	/* Shutdown the hardware interface(s) */
	api::shutdownHardware();

	return(0);
}

/* Split (tokenize) a command line */
char **tokenizeCommandLine(char *line) {
	const char *TOKEN_DELIMITER = " \t\r\n\a";
	int bufsize = MAX_COMMAND_LENGTH+1, position = 0;
	char **tokens = (char **)malloc(bufsize * sizeof(char*));
	char *token;

	if (!tokens) {
		errorHandler(0xC0000000);
		exit(-1);
	}

	token = strtok(line, TOKEN_DELIMITER);
	while (token != NULL) {
		tokens[position++] = token;

		if (position >= bufsize) {
			bufsize += MAX_COMMAND_LENGTH;
			tokens = (char **)realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) {
				errorHandler(0xC0000001);
				exit(-1);
			}
		}

		token = strtok(NULL, TOKEN_DELIMITER);
	}
	tokens[position] = NULL;

	return tokens;
}

/* Execute built-in command */
void executeCommand(char **args) {
	size_t i;
	int result, argv;

	if ((args[0] == NULL) || (args[0][0] == '|')) {
		// An empty command or a remark was entered.
		return;
	}

	/* Calculate the number of arguments to pass */
	argv = 0;
	while (args[argv] != NULL)
		argv++;

	for (i = 0; i < totalNumOfCommands(); i++) {
		if (strcmp(strtoupper(args[0]), commands[i].command) == 0) {
			switch (result = (*commands[i].function)(argv, args)) {
				case 0 :
					/* Function executed the command correctly */
					break;

				case -1 :
					/* User requested to exit */
					bye = true;
					break;

				case -2 :
					/* Syntax error: show help message */
					printf ("Usage: %s%s %s\n", PROMPT, commands[i].command, commands[i].help);
					break;

				default :
					/* Any other error returned by the function */
					errorHandler(result);
					break;
			}
			return;
		}
	}

	errorHandler(0x000000FE);
}

