/* commands.cpp
 * All * commands available on the FileStore command prompt
 *
 * (c) Eelco Huininga 2017-2018
 */

#include "commands.h"

const char *cmds[][2] = {
	{"ACCESS",	" <filename> (RWL)"},
	{"BYE",		""},
	{"CAT",		""},
	{"CDIR",	" <dir>"},
	{"CLOCK",	" <ON|OFF|AUTO>"},
	{"CONFIGURE",	" <keyword> (value)"},
	{"DATE",	""},
	{"DELETE",	" <filename>"},
	{"DISCS",	""},
	{"DISMOUNT",	""},
	{"EXIT",	""},
	{"HELP",	" (command)"},
	{"MOUNT",	" <filename>"},
	{"NETMON",	""},
	{"NEWUSER",	" <username> <password>"},
	{"NOTIFY",	" <station id> <message>"},
	{"PASS",	" <username> <password>"},
	{"PRINTTEST",	""},
	{"PRIV",	" <username> (S)"},
	{"REMUSER",	" <username>"},
	{"TIME",	""},
	{"USERS"	" (mask)"}
};

int (*cmds_jumptable[]) (char **) = {
	&commands::access,
	&commands::exit,
	&commands::cat,
	&commands::cdir,
	&commands::clock,
	&commands::configure,
	&commands::date,
	&commands::star_delete,
	&commands::discs,
	&commands::dismount,
	&commands::exit,
	&commands::help,
	&commands::mount,
	&commands::netmon,
	&commands::newuser,
	&commands::notify,
	&commands::pass,
	&commands::printtest,
	&commands::priv,
	&commands::remuser,
	&commands::star_time,
	&commands::users
};

int totalNumOfCommands(void) {
	return sizeof(cmds) / sizeof(char *) / 2;
}

