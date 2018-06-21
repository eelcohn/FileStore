/* configure.cpp
 * *CONFIGURE command handler
 *
 * (c) Eelco Huininga 2017
 */

#include <cstdio>			// Included for printf(), FILE*, fopen() and fclose()
#include <cstdlib>			// Included for strtol()
#include <cstring>			// Included for strcmp()

#include "star_configure.h"
#include "../main.h"			// strtoupper()

using namespace std;



namespace configuration {
	unsigned char	*servername = (unsigned char *)"FILESTOR";
	unsigned char	*printername = (unsigned char *)"RPIPRN";
	unsigned char	econet_station = 254;
	unsigned char	econet_network = 1;
	unsigned char	aun_station;
	unsigned char	aun_network;
	unsigned char	autolearn = 0;					// Autolearning for !Stations file is OFF (1=SESSION: only for this session, do not update !Stations / 2=FULL: add new stations to !Stations file)
	bool		relay_only_known_networks = false;
	unsigned char	*volume = NULL;
	unsigned char	*printqueue = (unsigned char *)"/usr/bin/lpr";
	bool		clock = false;
	unsigned int	clockspeed = 30000;
	unsigned short	dutycycle = 25;
	unsigned int	protectionLevel;
	unsigned char	*networkLogFile = (unsigned char *)"network.log";
	char	onError[256] = "%s &%08X: %s\n";
}

namespace commands {
	int configure(char **args) {
		int value;
		FILE *fp_printer;

		strtoupper(args[1]);
	
		if (args[1] == NULL) {
			printf("STATION         %i\n", configuration::econet_station);
			printf("NETWORK         %i\n", configuration::econet_network);
			printf("AUNNETWORK      %i\n", configuration::aun_network);
			printf("AUTOLEARN       %i\n", configuration::autolearn);
			printf("VOLUME          %s\n", configuration::volume);
			printf("PRINTQUEUE      %s\n", configuration::printqueue);
			if (configuration::clock == true)
				printf("CLOCK           ON\n");
			else
				printf("CLOCK           OFF\n");
			printf("CLOCKSPEED      %iHz\n", configuration::clockspeed);
			printf("DUTYCYCLE       %i%%\n", configuration::dutycycle);
			printf("ONERROR         %s\n", configuration::onError);
		} else if (strcmp(args[1], "STATION") == 0) {
			value = strtol(args[2], NULL, 10);
			if ((value < 1) || (value > 254)) {
				printf("Error: %s is an invalid station ID\n", args[2]);
				return(0x000000FD);
			} else {
				configuration::econet_station = value;
				printf("Econet station ID set to %i\n", value);
			}
		} else if (strcmp(args[1], "NETWORK") == 0) {
			value = strtol(args[2], NULL, 10);
			if ((value < 1) || (value > 254)) {
				printf("Error: %s is an invalid network ID\n", args[2]);
				return(0x000000FD);
			} else {
				configuration::econet_network = value;
				printf("Econet network ID set to %i\n", value);
			}
		} else if (strcmp(args[1], "AUNNETWORK") == 0) {
			value = strtol(args[2], NULL, 10);
			if ((value < 1) || (value > 254)) {
				printf("Error: %s is an invalid network ID\n", args[2]);
				return(0x000000FD);
			} else {
				configuration::aun_network = value;
				printf("AUN network ID set to %i\n", value);
			}
		} else if (strcmp(args[1], "PRINTQUEUE") == 0) {
			if (!(fp_printer = fopen(args[2], "w"))) {
				printf("Error: Could not open %s\n", args[2]);
				return(0x000000FD);
			} else {
				fclose (fp_printer);
				configuration::printqueue = (unsigned char *)args[2];
				printf("Printqueue set to %s\n", args[2]);
			}
		} else if (strcmp(args[1], "CLOCKSPEED") == 0) {
			value = strtol(args[2], NULL, 10);
			if ((value < 50000) || (value > 1000000)) {
				printf("Error: %s is an invalid clock speed value\n", args[2]);
				return(0x000000FD);
			} else {
				configuration::clockspeed = value;
				printf("Clockspeed set to %i Hz\n", value);
			}
		} else if (strcmp(args[1], "DUTYCYCLE") == 0) {
			value = strtol(args[2], NULL, 10);
			if ((value < 1) || (value > 99)) {
				printf("Error: %s is an invalid duty cycle value\n", args[2]);
				return(0x000000FD);
			} else {
				configuration::dutycycle = value;
				printf("Dutycycle set to %i%%\n", value);
			}
		} else if (strcmp(args[1], "ONERROR") == 0) {
			if (strlen(args[2]) > 256) {
				printf("Error: String too long\n");
				return(0x000000FD);
			} else {
				strcpy(configuration::onError, args[2]);
				printf("ON ERROR handler set to %s\n", args[2]);
			}
		} else
			return(0x000000FE);
		return(0);
	}
};


