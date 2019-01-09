/* cli.h
 * Handler for all Command Line Interpreter commands
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_CLI_HEADER
#define ECONET_CLI_HEADER

#include "econet.h"		// econet::Frame

#define BUFFER_LENGTH	128

//typedef int filestore_icpfunc_t (char **);

typedef struct {
//	filestore_icpfunc_t  *function;
	int		(*function)(int, char **);
	const char	*module;
	const char	*command;
	const char	*help;
} Command;

//extern const char	*cmds[][4];
//extern int		(*cmds_jumptable[]) (char **);
extern Command commands[];

namespace cli {
	extern unsigned int user_id;

	int access(int argv, char **args);
	int cat(int argv, char **args);
	int cdir(int argv, char **args);
	int clock(int argv, char **args);
	int clockspeed(int argv, char **args);
	int configure(int argv, char **args);
	int date(int argv, char **args);
	int del(int argv, char **args);
	int discs(int argv, char **args);
	int dismount(int argv, char **args);
	int ex(int argv, char **args);
	int exit(int argv, char **args);
	int help(int argv, char **args);
	int info(int argv, char **args);
	int login(int argv, char **args);
	int logout(int argv, char **args);
	int mount(int argv, char **args);
	int netmon(int argv, char **args);
	int newuser(int argv, char **args);
	int notify(int argv, char **args);
	int pass(int argv, char **args);
	int printtest(int argv, char **args);
	int priv(int argv, char **args);
	int remuser(int argv, char **args);
	int rename(int argv, char **args);
	int sessions(int argv, char **args);
	int star_time(int argv, char **args);
	int stations(int argv, char **args);
	int users(int argv, char **args);
	char **command_completion(const char *text, int start, int end);
	char *command_generator(const char *text, int state);
	int getPassword(const char *prompt, char *password, size_t pwlen);
}

void netmonPrintFrame(const char *interface, bool tx, econet::Frame *frame, int size);
size_t totalNumOfCommands(void);
#endif

