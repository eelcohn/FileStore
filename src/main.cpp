/* main.cpp
 * Econet gateway server: An Econet file- and print server with transparent ethernet-gateway functionality
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdlib>			// Included for malloc() and free()
#include <csignal>			// Included for signal() and SIGxxx definitions
#include <cstdio>			// Included for NULL, printf(), FILE*, fopen(), fgets(), feof() and fclose()
#include <cstring>			// Included for strcmp() and strlen()
#include <atomic>			// Included for std::atomic
#include <thread>			// Included for std::thread 
#include <openssl/sha.h>		// Included for SHA256_DIGEST_LENGTH

#include "errorhandler.h"		// Error handling functions
#include "main.h"			// Header file for this C++ module
#include "econet.h"			// Included for pollEconet() thread
#include "ethernet.h"			// Included for pollEthernet() thread
#include "commands/commands.h"		// All * commands
#include "platforms/platform.h"		// All platform-dependant functions

using namespace std;

std::atomic<bool>	bye;
bool			bootdone;
FILE			*fp_volume;
FILE			*fp_bootfile;
User			*users;
Station			*stations;
Disc			discs[ECONET_MAX_DISCDRIVES];



int main(int argc, char** argv) {
	char	*command = (char *) malloc(MAX_COMMAND_LENGTH);
	char	**args = NULL;
	int	result, cmdstart;

	/* Print startup message */
	printf("%s v%s.%s.%s\n\n", STARTUP_MESSAGE, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCHLEVEL);

	/* Initialize signal handlers */
	if (signal(SIGHUP, sigHandler) == SIG_ERR)
		errorHandler(0xFFFFFFFF, "Can't catch SIGHUP");
	if (signal(SIGINT, sigHandler) == SIG_ERR)
		errorHandler(0xFFFFFFFF, "Can't catch SIGINT");
	if (signal(SIGTERM, sigHandler) == SIG_ERR)
		errorHandler(0xFFFFFFFF, "Can't catch SIGTERM");

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
	if (!loadUsers()) {
		errorHandler(0x00000021, "Cannot find password file");
		exit(0x00000021);
	}

	/* Load !Stations (Econet to/from Ethernet translation table) */
	loadStations();

	/* Load !Boot */
	if (!(fp_bootfile = fopen(BOOTFILE, "r"))) {
		bootdone = true;
	} else {
		printf("- Execing %s:\n", BOOTFILE);
		bootdone = false;
	}

	/* Main loop */
	bye = false;
	econet::netmon = false;

	/* Spawn new thread for polling hardware and processing network data */
	std::thread thread_EconetpollNetworkReceive(econet::pollNetworkReceive);
	std::thread thread_EthernetpollNetworkReceive(ethernet::pollAUNNetworkReceive);
#ifdef OPENSSL
	std::thread thread_EthernetSecureAUNListener(ethernet::dtls_SAUNListener);
#endif

	/* Announce that a new Econet bridge is online on the network */
	sendBridgeAnnounce();

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
				printf("Could not read stdin.");
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

/*
#ifdef DEBUGBUILD
// Temporary include for debugging purposes (D00, D01 ... D80 etc)
#include "gpio.h"

	if (strcmp(args[0], "D00") == 0) {
		gpio::writeRegister(0, 0x00);
	} else

	if (strcmp(args[0], "D01") == 0) {
		gpio::writeRegister(0, 0x01);
	} else

	if (strcmp(args[0], "D02") == 0) {
		gpio::writeRegister(1, 0x02);
	} else

	if (strcmp(args[0], "D04") == 0) {
		gpio::writeRegister(2, 0x04);
	} else

	if (strcmp(args[0], "D08") == 0) {
		gpio::writeRegister(3, 0x08);
	} else

	if (strcmp(args[0], "D10") == 0) {
		gpio::writeRegister(0, 0x10);
	} else

	if (strcmp(args[0], "D20") == 0) {
		gpio::writeRegister(1, 0x20);
	} else

	if (strcmp(args[0], "D40") == 0) {
		gpio::writeRegister(2, 0x40);
	} else

	if (strcmp(args[0], "D80") == 0) {
		gpio::writeRegister(3, 0x80);
	} else
#endif
*/

void sigHandler(int sig) {
	switch (sig) {
		case SIGHUP :
			printf("received SIGHUP\n");
			exit(-1);
			break;

		case SIGINT :					// <ctrl>+C was pressed
			printf("received SIGINT\n");
			bye = true;
			break;

		case SIGTERM :
			printf("received SIGTERM\n");
			bye = true;
			// This program will hang on SIGTERM because fgets(), scanf() and cin>> in the main loop don't handle SIGTERMs correctly
			// Should find another solution to deal with this.. Ideally the program should exit by the return in the main() function
//			commands::dismount(NULL);
//			exit(0);
			break;

		case SIGUSR1 :
			printf("received SIGUSR1\n");
			break;

		default :
			printf("received unknown signal %i\n", sig);
			break;
	}
}

/* Load !Users file */
bool loadUsers(void) {
	int i = 0, result;
	FILE *fp_usersfile;
	char a[256], b[256], c[256];

	printf("- Loading %s: ", USERSFILE);

	fp_usersfile = fopen(USERSFILE, "r");
	if (fp_usersfile != NULL) {
		while ((result = fscanf(fp_usersfile, "%s %s %s", a, b, c)) != EOF) {
			if ((result == 3) && (a[0] != '#')) {
				i++;
			}
		}
		fclose(fp_usersfile);
		printf("    %i users loaded.\n", i);
	} else {
		return(false);
	}
	return(true);
}

/* Load !Stations file */
void loadStations(void) {
	int i = 0, result, a, b, d;
	char buffer[256];
	char c[20];
	FILE *fp_stationsfile;
	fpos_t pos;
	in_addr addr;

	printf("- Loading %s: ", STATIONSFILE);

	fp_stationsfile = fopen(STATIONSFILE, "r");
	if (fp_stationsfile != NULL) {
		while (!feof(fp_stationsfile)) {
			fgetpos(fp_stationsfile, &pos);
			if (fgets(buffer, sizeof(buffer), fp_stationsfile) != NULL) {
				if (buffer[0] != '#') {
					fsetpos(fp_stationsfile, &pos);
					result = fscanf(fp_stationsfile, "%i %i %s %i", &a, &b, c, &d);
					if (result == 4) {
						if ((a < 0) || (a > 127)) {
							fprintf(stderr, "Invalid econet network value: %i\n", a);
							continue;
						}
						if ((b < 1) || (b > 254)) {
							fprintf(stderr, "Invalid econet station value: %i\n", b);
							continue;
						}
						if (inet_aton(c, &addr) == 0) {
							fprintf(stderr, "Invalid IP address: %s\n", c);
							continue;
						}
						if ((b < 1) || (b > 65535)) {
							fprintf(stderr, "Invalid port number: %i\n", d);
							continue;
						}
						i++;
					}
				}
			}
		}
		fclose(fp_stationsfile);
		printf(" %i stations loaded.\n", i);
	} else {
		errorHandler(0x000000FF, "Could not load !Stations file");
	}
}

/* Split (tokenize) a command line */
#define LSH_TOK_BUFSIZE 64
char **tokenizeCommandLine(char *line) {
	const char *TOKEN_DELIMITER = " \t\r\n\a";
	int bufsize = LSH_TOK_BUFSIZE, position = 0;
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
			bufsize += LSH_TOK_BUFSIZE;
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

int totalNumOfUsers(void) {
	return sizeof(User) / sizeof(User);
}

/* Send a broadcast frame to announce that a new bridge is available */
#include "configuration.h"
void sendBridgeAnnounce(void) {
	econet::Frame frame;

	frame.data[0x00]	= 0xFF;
	frame.data[0x01]	= 0xFF;
	frame.data[0x02]	= configuration::econet_network;
	frame.data[0x03]	= configuration::econet_station;
	frame.data[0x04]	= 0x80;
	frame.data[0x05]	= 0x9C;
	frame.data[0x06]	= configuration::ethernet_network;

	econet::transmitFrame(&frame, 7);

	frame.data[0x02]	= configuration::ethernet_network;
	frame.data[0x03]	= configuration::ethernet_station;
	frame.data[0x06]	= configuration::econet_network;

	ethernet::transmitFrame(&frame, 7);
#ifdef OPENSSL
	ethernet::transmitSecureAUNFrame(&frame, 7);
#endif
}

