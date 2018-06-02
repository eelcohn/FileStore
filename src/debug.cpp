/* debug.h
 * Debug and logging options
 *
 * (c) Eelco Huininga 2017
 */

#include <cstdio>			// Included for printf, FILE, fopen, fclose
#include "configuration.h"		// Included for global configuration

namespace debug {
	FILE *fp_networkLog;

	void networkLogStart(void) {
//		fp_networkLog = fopen(configure::networkLogFile, "w+");
		fp_networkLog = fopen("network.log", "w+");
		printf("i/f Di  Src   Dst  Ct Pt  Data                     ASCII   \n");
		printf("       Ne:St Ne:St                                         \n");
		printf("-----------------------------------------------------------\n");
	}

	void networkLog(void) {
		printf("eco Rx 00:D4 FF:FF 80 D0  2A 44 45 4C 45 54 45 00  *DELETE.\n");
	}

	void networkLogStop(void) {
		fclose(fp_networkLog);
	}
}

