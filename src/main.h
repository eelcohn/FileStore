/* main.h
 * Econet gateway server
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_MAIN_HEADER
#define ECONET_MAIN_HEADER

#define MAX_COMMAND_LENGTH		128
#define STARTUP_MESSAGE			"Econet FileStore"
#define PROMPT				"*"
#define BOOTFILE			"./conf/!Boot"
#define STATIONSFILE			"./conf/!Stations"
#define USERSFILE			"./conf/!Users"
#define PRINTBUFFERFILE			"/tmp/FileStore.printbuffer.tmp"
#define FILESERVERTMPFILE		"/tmp/FileStore.fileserver.tmp"
#define ECONET_MAX_DISCDRIVES		8
#define ECONET_MAX_DISCTITLE_LEN	16
#define ECONET_MAX_FILENAME_LEN		10
#define ECONET_MAX_DIRENTRIES		47
#define FILESTORE_MAX_ATTRIBS		8

#include <cstdio>			// Included for FILE*
#include <atomic>			// Included for std::atomic
#include <arpa/inet.h>			// Included for in_addr
#include <openssl/sha.h>		// Included for SHA256_DIGEST_LENGTH

#include "config.h"			// All definitions made by the ./configure or ./build script
#include "platforms/platform.h"		/* PATH_MAX */

enum FileSystems {NATIVE_FS, ACORN_DFS, ACORN_ADFS, WATFORD_DDFS, SOLIDISK_DDFS};
/*const char *FileSystemsDesc[] = {
	"NativeFS",
	"DFS",
	"ADFS",
	"WatfordDDFS",
	"SolidiskDDFS"
};
*/
typedef struct {
	uint32_t	filesystem;				/* Type of file system, see enum above */
	char		title[ECONET_MAX_DISCTITLE_LEN];	/* Disc title */
	char		localobj[PATH_MAX];			/* Full path to object on local filesystem */
	FILE		*fp;
	char		*image;					/* Pointer to */
	uint32_t	size;					/* Total size of disc */
} Disc;

typedef struct {
	uint8_t		disc_id;	// Disc ID where the file is located
	uint8_t		dir;		// Directory where the file is located
	uint8_t		file;		// File ID of the file
	uint32_t	ptr;		// Pointer for BGET, BPUT, OSGBPB etc.
} FileHandles;

extern std::atomic<bool>	bye;
extern FILE			*fp_volume;
extern Disc			*discs[ECONET_MAX_DISCDRIVES];



bool	loadUsers(void);
void	loadStations(void);
void	sendBroadcastFrame(void);
char	**tokenizeCommandLine(char *line);
void	executeCommand(char **args);
void	sendBridgeAnnounce(void);

#endif

