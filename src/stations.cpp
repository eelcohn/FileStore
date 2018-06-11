/* stations.cpp
 * All Stations related functions
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>			// Included for EOF, printf(), FILE*, fopen(), fgets(), feof() and fclose()
#include <cstring>			// Included for memcpy(), strlen()
#include <arpa/inet.h>			// Included for in_addr

#include "stations.h"			// 
#include "main.h"			// Included for main.h


using namespace std;



namespace stations {
//	Station *stations;
	Station stations[127][255];
	int totalStations = 0;

	/* Load !Stations file */
	bool loadStations(void) {
		int result, n, s, p;
		char buffer[256];
		char c[20];
		FILE *fp_stationsfile;
		fpos_t pos;
		in_addr addr;

		printf("- Loading %s: ", STATIONSFILE);

		fp_stationsfile = fopen(STATIONSFILE, "r");
		if (fp_stationsfile != NULL) {
			while (!feof(fp_stationsfile)) {
				fgetpos(fp_stationsfile, &pos);
				if (fgets(buffer, sizeof(buffer), fp_stationsfile) != NULL) {
					// Skip all comments
					if (buffer[0] != '#') {
						fsetpos(fp_stationsfile, &pos);
						result = fscanf(fp_stationsfile, "%i %i %s %i", &n, &s, c, &p);
						if (result == 4) {
							if ((n < 0) || (n > 127)) {
								fprintf(stderr, "Invalid econet network value: %i\n", n);
								continue;
							}
							if ((s < 1) || (s > 254)) {
								fprintf(stderr, "Invalid econet station value: %i\n", s);
								continue;
							}
							if (inet_aton(c, &addr) == 0) {
								fprintf(stderr, "Invalid IP address: %s\n", c);
								continue;
							}
							if ((p < 1) || (p > 65535)) {
								fprintf(stderr, "Invalid port number: %i\n", p);
								continue;
							}
							stations[n][s].addr = addr;
							stations[n][s].port = p;
//							printf("%i:%i IP=%08X port=%i\n", n, s, stations[n][s].addr.s_addr, stations[n][s].port);
							stations::totalStations++;

						}
					}
				}
			}
			fclose(fp_stationsfile);
			printf(" %i stations loaded.\n", stations::totalStations);
		} else {
			return(false);
		}
		return(true);
	}
}
