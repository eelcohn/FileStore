/* cli.h
 * Handler for all Command Line Interpreter commands
 *
 * (c) Eelco Huininga 2017-2018
 */

#ifndef ECONET_CLI_HEADER
#define ECONET_CLI_HEADER

#include "econet.h"		// econet::Frame

#define BUFFER_LENGTH	128

extern const char	*cmds[][2];
extern int		(*cmds_jumptable[]) (char **);

namespace commands {
	int clock(char **args);
	int clockspeed(char **args);
	int configure(char **args);
	int date(char **args);
	int discs(char **args);
	int dismount(char **args);
	int exit(char **args);
	int help(char **args);
	int mount(char **args);
	int netmon(char **args);
	int newuser(char **args);
	int notify(char **args);
	int pass(char **args);
	int printtest(char **args);
	int priv(char **args);
	int remuser(char **args);
	int star_time(char **args);
	int users(char **args);
}

void netmonPrintFrame(const char *interface, bool tx, econet::Frame *frame, int size);
bool kbhit(void);
int totalNumOfCommands(void);
#endif

