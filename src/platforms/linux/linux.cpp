/* linux.cpp
 * Linux-specific functions
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <csignal>			// Included for signal() and SIGxxx definitions
#include <cstdio>			// Included for

#include "linux.h"			//
#include "../../errorhandler.h"		// Error handling functions
#include "../../main.h"			// Included for bye



/* Initialize SIGNAL handlers */
void initSignals(void) {
	if (signal(SIGHUP, sigHandler) == SIG_ERR)
		errorHandler(0xC0000002);
	if (signal(SIGINT, sigHandler) == SIG_ERR)
		errorHandler(0xC0000003);
	if (signal(SIGTERM, sigHandler) == SIG_ERR)
		errorHandler(0xC0000004);
}

/* Signal handler */
void sigHandler(int sig) {
	switch (sig) {
		case SIGHUP :
			printf("received SIGHUP\n");
//			exit(-1);
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

/* Send a string to a printer */
int print(char *buffer)
{
//	FILE *printer = popen("/usr/bin/lpr -P PDF", "w");
	FILE *printer = popen("lpr", "w");

	if (printer == NULL)
		return (-1);

	fputs(buffer, printer);
	pclose(printer);

	return(0);
}

