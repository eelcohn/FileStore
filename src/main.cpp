/* main.cpp
 * Econet gateway server: An Econet file- and print server with transparent ethernet-gateway functionality
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>			// Included for NULL, printf(), FILE*, fopen(), fgets(), feof() and fclose()
#include <cstring>			// Included for memmove(), strtok(), strcmp() and strlen()
#include <atomic>			// Included for std::atomic
#include <thread>			// Included for std::thread

#include "errorhandler.h"		// Error handling functions
#include "econet.h"			// Included for pollEconet() thread
#include "ethernet.h"			// Included for pollEthernet() thread
#include "main.h"			// Header file for this C++ module
#include "users.h"			// Included for users::loadUsers()
#include "stations.h"			// Included for users::loadStations()
#include "commands/commands.h"		// All * commands
#include "platforms/platform.h"		// All platform-dependant functions

using namespace std;

std::atomic<bool>	bye;
bool			bootdone;
FILE			*fp_volume;
FILE			*fp_bootfile;
Disc			discs[ECONET_MAX_DISCDRIVES];



int main(int argc, char** argv) {
	char	*command = (char *) malloc(MAX_COMMAND_LENGTH);
	char	**args = NULL;
	int	result, cmdstart;

	/* Print startup message */
	printf("%s v%s.%s.%s\n\n", STARTUP_MESSAGE, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCHLEVEL);

	/* Initialize signal handlers */
	initSignals();

	/* Initialize the RaspberryPi GPIO hardware */
	if ((result = api::initializeHardware()) != 0) {
		printf("Warning &%08X: could not initialize hardware", result);
		exit(result);
	}

	/* Reset the Econet module */
	if ((result = api::resetHardware()) != 0) {
		errorHandler(result, "Could not reset hardware");
		exit(result);
	}

	/* Load !Users (users and passwords) */
	if (!users::loadUsers()) {
		errorHandler(0x00000021, "Cannot find password file");
		exit(0x00000021);
	}

	/* Load !Stations (Econet to/from Ethernet translation table) */
	if (!stations::loadStations()) {
		errorHandler(0x000000FF, "Could not load !Stations file");
		exit(0x000000FF);
	}

	/* Load !Boot */
	fp_bootfile = fopen(BOOTFILE, "r");
	if (fp_bootfile == NULL) {
		bootdone = true;
	} else {
		printf("- Execing %s:\n", BOOTFILE);
		bootdone = false;
	}

	/* Spawn new thread for polling hardware and processing network data */
	std::thread thread_EconetpollNetworkReceive(econet::pollNetworkReceive);
	std::thread thread_EthernetpollNetworkReceive(ethernet::pollAUNNetworkReceive);
#ifdef OPENSSL
	std::thread thread_EthernetSecureAUNListener(ethernet::dtls_SAUNListener);
#endif

	/* Announce that a new Econet bridge is online on the network */
	econet::sendBridgeAnnounce();

	/* Scan the Econet for existing bridges and networks */
	econet::sendWhatNetBroadcast();

	/* Main loop */
	bye = false;
	econet::netmon = false;

	while (bye == false) {
		/* Check network status */
		if (api::networkState())
			errorHandler(0x000003A3, "No clock signal detected on the Econet network");

		printf(PROMPT);
		if (bootdone == false) {
			if (fgets(command, MAX_COMMAND_LENGTH, fp_bootfile) != NULL) {
				if (feof(fp_bootfile)) {

					fclose(fp_bootfile);
					bootdone = true;
				}
			} else {
				fclose(fp_bootfile);
				bootdone = true;
			}
		} else {
//			cin >> command;
//			scanf("%80s", command);
			if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL)
				printf("Could not read stdin.\n");
		}

		/* Strip leading *'s */
		cmdstart = 0;
		while (command[cmdstart] == PROMPT[0])
			cmdstart++;

		if (cmdstart != 0)
			memmove(command, (command + cmdstart), strlen(command));

		/* When processing the !Boot script, print the command currently being processed */
		if (bootdone == false)
			printf("%s", command);

		/* Split command line into tokens */
		args = tokenizeCommandLine(command);

		/* Execute command */
		executeCommand(args);
	}

	printf("\n");

	/* Wait for the threads to finish */
	thread_EconetpollNetworkReceive.join();
	thread_EthernetpollNetworkReceive.join();

	/* Dismount all open disc images */
	commands::dismount(NULL);

	/* Release memory */
	free(command);
	free(args);

	return(0);
}

/* Split (tokenize) a command line */
char **tokenizeCommandLine(char *line) {
	const char *TOKEN_DELIMITER = " \t\r\n\a";
	int bufsize = MAX_COMMAND_LENGTH, position = 0;
	char **tokens = (char **)malloc(bufsize * sizeof(char*));
	char *token;

	if (!tokens) {
		errorHandler(0xFFFFFFFF, "tokenizeCommandLine: allocation error");
		exit(-1);
	}

	token = strtok(line, TOKEN_DELIMITER);
	while (token != NULL) {
		tokens[position++] = token;

		if (position >= bufsize) {
			bufsize += MAX_COMMAND_LENGTH;
			tokens = (char **)realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) {
				errorHandler(0xFFFFFFFF, "tokenizeCommandLine: allocation error");
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
	int i, result;

	if ((args[0] == NULL) || (args[0][0] == '|')) {
		// An empty command or a remark was entered.
		return;
	}

	for (i = 0; i < totalNumOfCommands(); i++) {
		if (strcmp(args[0], cmds[i][0]) == 0) {
			switch (result = (*cmds_jumptable[i])(args)) {
				case 0 :
					break;

				case -1 :
					bye = true;
					break;

				case -2 :
					printf ("%s\n", cmds[i][1]);
					break;

				default :
					errorHandler(result, "Bad command");
					break;
			}
			return;
		}
	}

	errorHandler(0x000000FE, "Bad command");
}

