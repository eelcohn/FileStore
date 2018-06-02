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
#define BOOTFILE			"!Boot"
#define STATIONSFILE			"!Stations"
#define USERSFILE			"!Users"
#define ECONET_MAX_DISCDRIVES		8

#include <cstdio>			// Included for FILE*
#include <atomic>			// Included for std::atomic
#include <arpa/inet.h>			// Included for in_addr

#include <openssl/sha.h>		// Included for SHA256_DIGEST_LENGTH

typedef struct {
	char username[16];
	char pwhash[SHA256_DIGEST_LENGTH];
	char access[8];
} User;

typedef struct {
	struct in_addr addr;
	unsigned short	port;
	unsigned char	network;
	unsigned char	station;
} Station;

typedef struct {
	unsigned char title[16];
	unsigned char bootoption;
} Disc;

extern std::atomic<bool>	bye;
extern FILE			*fp_volume;
extern Disc			discs[ECONET_MAX_DISCDRIVES];



void	sigHandler(int sig);
bool	loadUsers(void);
void	loadStations(void);
void	sendBroadcastFrame(void);
char	**tokenizeCommandLine(char *line);
void	executeCommand(char **args);
int	totalNumOfCommands(void);
void	sendBridgeAnnounce(void);

#endif

