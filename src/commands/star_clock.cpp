/* clock.cpp
 * *CLOCK command handler
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>			// Included for printf()
#include <cstring>			// Included for strcmp()
#include "../platforms/platform.h"	// startClock() and stopClock()
#include "../settings.h"		// Global configuration variables are defined here
#include "../main.h"			// strtoupper()

using namespace std;



namespace commands {
	int clock(char **args) {
		if (args[1] == NULL) {
			return(-2);
		} else {
			strtoupper(args[1]);
			if (strcmp(args[1], "ON") == 0) {
				api::startClock();
				printf("Internal clock is now on (%iHz %i%%).\n", configuration::clockspeed, configuration::dutycycle);
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
}


