/* cli.cpp
 * Handler for all Command Line Interpreter commands
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>			// NULL, printf(), sprintf(), FILE*, fopen(), fgets(), feof() and fclose()
#include <cstdlib>			// strtol()
#include <cstring>			// strlen(), strncpy()
#include <ctime>			// time_t tm
#include <openssl/sha.h>		// SHA256_CTX, SHA256_Init, SHA256_Update, SHA256_Final and SHA256_DIGEST_LENGTH
#include "cli.h"
#include "config.h"			// DEBUG_BUILD
#include "debug.h"			// debug::*
#include "econet.h"			// econet::netmon and econet::Frame
#include "main.h"			// bye, strtoupper() and STARTUP_MESSAGE
#include "netfs.h"			// netfs::*
#include "platforms/platform.h"
#include "settings.h"			// settings::*


extern std::atomic<bool>	bye;
extern FILE			*fp_volume;

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
	&netfs::access,
	&commands::exit,
	&netfs::cat,
	&netfs::cdir,
	&commands::clock,
	&commands::clockspeed,	
	&commands::configure,
	&commands::date,
	&netfs::del,
	&commands::discs,
	&commands::dismount,
	&commands::exit,
	&commands::help,
	&netfs::i_am,
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

using namespace std;



namespace commands {
	int clock(char **args) {
		if (args[1] == NULL) {
			return(-2);
		} else {
			strtoupper(args[1]);
			if (strcmp(args[1], "ON") == 0) {
				api::startClock();
				printf("Internal clock is now on (%iHz %i%%).\n", settings::clockspeed, settings::dutycycle);
			} else if (strcmp(args[1], "OFF") == 0) {
				api::stopClock();
				printf("Internal clock is now off.\n");
			} else if (strcmp(args[1], "AUTO") == 0) {
				printf("CLOCK AUTO - Not yet implemented\n");
			} else {
				return(0x000000FE);
			}
		}
		return(0);
	}

	int clockspeed(char **args) {
		printf("Measured clockspeed is %i Hz\n", api::getClockSpeed());
		return(0);
	}

	int configure(char **args) {
		int value;
		FILE *fp_printer;

		strtoupper(args[1]);
	
		if (args[1] == NULL) {
			printf("STATION         %i\n", settings::econet_station);
			printf("NETWORK         %i\n", settings::econet_network);
			printf("AUNNETWORK      %i\n", settings::aun_network);
			printf("AUTOLEARN       %i\n", settings::autolearn);
			printf("VOLUME          %s\n", settings::volume);
			printf("PRINTQUEUE      %s\n", settings::printqueue);
			if (settings::clock == true)
				printf("CLOCK           ON\n");
			else
				printf("CLOCK           OFF\n");
			printf("CLOCKSPEED      %iHz\n", settings::clockspeed);
			printf("DUTYCYCLE       %i%%\n", settings::dutycycle);
			printf("ONERROR         %s\n", settings::onError);
		} else if (strcmp(args[1], "STATION") == 0) {
			value = strtol(args[2], NULL, 10);
			if ((value < 1) || (value > 254)) {
				printf("Error: %s is an invalid station ID\n", args[2]);
				return(0x000000FD);
			} else {
				settings::econet_station = value;
				printf("Econet station ID set to %i\n", value);
			}
		} else if (strcmp(args[1], "NETWORK") == 0) {
			value = strtol(args[2], NULL, 10);
			if ((value < 1) || (value > 254)) {
				printf("Error: %s is an invalid network ID\n", args[2]);
				return(0x000000FD);
			} else {
				settings::econet_network = value;
				printf("Econet network ID set to %i\n", value);
			}
		} else if (strcmp(args[1], "AUNNETWORK") == 0) {
			value = strtol(args[2], NULL, 10);
			if ((value < 1) || (value > 254)) {
				printf("Error: %s is an invalid network ID\n", args[2]);
				return(0x000000FD);
			} else {
				settings::aun_network = value;
				printf("AUN network ID set to %i\n", value);
			}
		} else if (strcmp(args[1], "PRINTQUEUE") == 0) {
			if (!(fp_printer = fopen(args[2], "w"))) {
				printf("Error: Could not open %s\n", args[2]);
				return(0x000000FD);
			} else {
				fclose (fp_printer);
				settings::printqueue = (unsigned char *)args[2];
				printf("Printqueue set to %s\n", args[2]);
			}
		} else if (strcmp(args[1], "CLOCKSPEED") == 0) {
			value = strtol(args[2], NULL, 10);
			if ((value < 50000) || (value > 1000000)) {
				printf("Error: %s is an invalid clock speed value\n", args[2]);
				return(0x000000FD);
			} else {
				settings::clockspeed = value;
				printf("Clockspeed set to %i Hz\n", value);
			}
		} else if (strcmp(args[1], "DUTYCYCLE") == 0) {
			value = strtol(args[2], NULL, 10);
			if ((value < 1) || (value > 99)) {
				printf("Error: %s is an invalid duty cycle value\n", args[2]);
				return(0x000000FD);
			} else {
				settings::dutycycle = value;
				printf("Dutycycle set to %i%%\n", value);
			}
		} else if (strcmp(args[1], "ONERROR") == 0) {
			if (strlen(args[2]) > 256) {
				printf("Error: String too long\n");
				return(0x000000FD);
			} else {
				strlcpy((char *)settings::onError, args[2], sizeof(settings::onError));
				printf("ON ERROR handler set to %s\n", args[2]);
			}
		} else
			return(0x000000FE);
		return(0);
	}

	int date(char **args) {
		char buffer[128];
		time_t rawtime;
		struct tm * timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime (buffer, BUFFER_LENGTH, "Today is %A the %dth of %B %Y\n", timeinfo);

		puts(buffer);
		return(0);
	}

	int discs(char **args) {
		return(0);
	}

	int dismount(char **args) {
		if(fp_volume != NULL) {
			fclose(fp_volume);
			fp_volume = NULL;
			settings::volume = NULL;
		}
		return(0);
	}

	int exit(char **args) {
		bye = true;
		return(0);
	}

	int help(char **args) {
		int i;

		printf("%s v%s.%s.%s\n\n", STARTUP_MESSAGE, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCHLEVEL);
		for (i = 0; i < (totalNumOfCommands()); i++) {
			printf("   %s%s\n", cmds[i][0], cmds[i][1]);
		}
		printf("\n");

		return(0);
	}

	int mount(char **args) {
		if ((fp_volume = fopen(args[1], "r")) != NULL) {
			settings::volume = (unsigned char *)args[1];
		} else {
			fp_volume = NULL;
			return(0x000000D6);	// D6 File not found
		}			
		return(0);
	}

	int netmon(char **args) {
		econet::netmon = true;
		printf("Netmon started\n");

		while (bye == false) {
			// The econet::pollNetworkReceive, econet::sendFrame, ethernet::pollNetworkReceive and ethernet::sendFrame will check if econet::netmon is set to true.
			// If it is, they will print the sent or received frame to stdin
			if (kbhit()) {
				getc(stdin);
				bye = true;
			}
		}

		econet::netmon = false;
		bye = false;
		printf("\n");
		printf(PROMPT);
		return(0);
	}

	int newuser(char **args) {
	 	char		outputBuffer[65];
		unsigned char	hash[SHA256_DIGEST_LENGTH];
		int		i;
		SHA256_CTX	sha256;

		SHA256_Init(&sha256);
		SHA256_Update(&sha256, args[2], strlen(args[2]));
		SHA256_Final(hash, &sha256);

		i = 0;
		for(i = 0; i < SHA256_DIGEST_LENGTH; i++) {
			sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
		}
		outputBuffer[64] = 0;
		printf("Added new user %s with password hash %s\n", args[1], outputBuffer);

		return(0);
	}

	int notify(char **args) {
		return(0);
	}

	int pass(char **args) {
		return(0);
	}

	int printtest(char **args) {
		return(0);
	}

	int priv(char **args) {
		return(0);
	}

	int remuser(char **args) {
		return(0);
	}

	int star_time(char **args) {
		char buffer[BUFFER_LENGTH];
		time_t rawtime;
		struct tm* timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(buffer, BUFFER_LENGTH, "Good afternoon! It's %H:%M:%S\n", timeinfo);

		puts(buffer);
		return(0);
	}

	int users(char **args) {
		return(0);
	}
}

/* Hex-dump a frame to stdout */
void netmonPrintFrame(const char *interface, bool tx, econet::Frame *frame, int size) {
	int i, addr;
	char valid[8];

	if (tx)
		printf("Tx");
	else
		printf("Rx");

	if (frame->status | ECONET_FRAME_INVALID)
		strlcpy(valid, "invalid", sizeof(valid));
	else
		strlcpy(valid, "valid  ", sizeof(valid));

	printf("  %s  %s  dst=%02X:%02X  src=%02X:%02X  ctrl=%02X  port=%02X  size=%i bytes\n", interface, valid, frame->dst_network, frame->dst_station, frame->src_network, frame->src_station, frame->control, frame->port, size);

	addr = 0;
	while ((size - addr) > 0) {
		printf("%04X  ", addr);
		for (i = addr; i < ((size - addr) & 0xf); i++) {
			printf("%02X ", frame->data[i]);
		}
		printf(" ");
		for (i = addr; i < ((size - addr) & 0xf); i++) {
			if ((frame->data[i] > 31) && (frame->data[i] < 127))
				printf("%c", frame->data[i]);
			else
				printf(".");
		}
		printf("\n");
		addr += 16;
	}
}

bool kbhit(void) {
	struct timeval tv;
	fd_set read_fd;

	/* Do not wait at all, not even a microsecond */
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	/* Must be done first to initialize read_fd */
	FD_ZERO(&read_fd);

	/* Makes select() ask if input is ready: 0 is the file descriptor for stdin */
	FD_SET(0,&read_fd);

	/* The first parameter is the number of the largest file descriptor to check + 1. */
	if(select(1, &read_fd,NULL, /*No writes*/NULL, /*No exceptions*/&tv) == -1)
		return(false); /* An error occured */

	/* read_fd now holds a bit map of files that are
	* readable. We test the entry for the standard
	* input (file 0). */

	if(FD_ISSET(0,&read_fd))
		/* Character pending on stdin */
		return(true);

	/* no characters were pending */
	return(false);
}

int totalNumOfCommands(void) {
	return sizeof(cmds) / sizeof(char *) / 2;
}

