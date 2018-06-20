/* main.h
 * Econet gateway server
 *
 * (c) Eelco Huininga 2017-2018
 */

#ifndef ECONET_MAIN_HEADER
#define ECONET_MAIN_HEADER

#define MAX_COMMAND_LENGTH		128
#define STARTUP_MESSAGE			"Econet FileStore"
#define VERSION_MAJOR			"0"
#define VERSION_MINOR			"00"
#define VERSION_PATCHLEVEL		"00"
#define PROMPT				"*"
#define BOOTFILE			"./conf/!Boot"
#define STATIONSFILE			"./conf/!Stations"
#define USERSFILE			"./conf/!Users"
#define ECONET_MAX_DISCDRIVES		8

#include <cstdio>			// Included for FILE*
#include <atomic>			// Included for std::atomic
#include <arpa/inet.h>			// Included for in_addr
#include <openssl/sha.h>		// Included for SHA256_DIGEST_LENGTH

#include "config.h"		// Any definitions made by the ./configure or ./build script

typedef struct {
	unsigned char title[16];
	unsigned char bootoption;
} Disc;

extern std::atomic<bool>	bye;
extern FILE			*fp_volume;
extern Disc			discs[ECONET_MAX_DISCDRIVES];



bool	loadUsers(void);
void	loadStations(void);
void	sendBroadcastFrame(void);
char	**tokenizeCommandLine(char *line);
void	executeCommand(char **args);
int	totalNumOfCommands(void);
void	sendBridgeAnnounce(void);
char *	strtoupper(char *s);

#endif

