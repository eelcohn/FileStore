/* commands.cpp
 * All * commands available on the FileStore command prompt
 *
 * (c) Eelco Huininga 2017-2018
 */

#include "../config.h"
#include "../debug.h"
#include "commands.h"

const char *cmds[][2] = {
#ifdef DEBUGBUILD
	{"READ",	" <reg>"},
	{"WRITE",	" <reg> <value>"},
	{"D",		" <0..7> <0|1>"},
	{"RS",		" <0..3>"},
	{"RW",		" <R|W>"},
	{"CS",		" <ON|OFF>"},
	{"RST",		" <ON|OFF>"},
	{"PHI2",	" <ON|OFF>"},
#endif
	{"ACCESS",	" <filename> (RWL)"},
	{"BYE",		""},
	{"CAT",		""},
	{"CDIR",	" <dir>"},
	{"CLOCK",	" <ON|OFF|AUTO>"},
	{"CLOCKSPEED",	""},
	{"CONFIGURE",	" <keyword> (value)"},
	{"DATE",	""},
	{"DELETE",	" <filename>"},
	{"DISCS",	""},
	{"DISMOUNT",	""},
	{"EXIT",	""},
	{"HELP",	" (command)"},
	{"I AM",	" <username>"},
	{"MOUNT",	" <filename>"},
	{"NETMON",	""},
	{"NEWUSER",	" <username> <password>"},
	{"NOTIFY",	" <station id> <message>"},
	{"PASS",	" <username> <password>"},
	{"PRINTTEST",	""},
	{"PRIV",	" <username> (S)"},
	{"REMUSER",	" <username>"},
	{"TIME",	""},
	{"USERS",	" (mask)"}
};

int (*cmds_jumptable[]) (char **) = {
#ifdef DEBUGBUILD
	&debug::read,
	&debug::write,
	&debug::d,
	&debug::rs,
	&debug::rw,
	&debug::cs,
	&debug::rst,
	&debug::phi,
#endif
	&commands::access,
	&commands::exit,
	&commands::cat,
	&commands::cdir,
	&commands::clock,
	&commands::clockspeed,	
	&commands::configure,
	&commands::date,
	&commands::del,
	&commands::discs,
	&commands::dismount,
	&commands::exit,
	&commands::help,
	&commands::i_am,
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

