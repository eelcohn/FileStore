/* stations.cpp
 * All Stations related functions
 *
 * (c) Eelco Huininga 2017-2019
 */

#include <cstdio>			// Included for EOF, printf(), FILE*, fopen(), fgets(), feof() and fclose()
#include <cstring>			// Included for memcpy(), strlen()
#include <arpa/inet.h>			// Included for in_addr

#include "stations.h"			// 
#include "main.h"			// Included for main.h

const char *station_type[] = {"", "Console", "Econet", "IPv4", "IPv6"};

using namespace std;



namespace stations {
//	Station *stations;
	Station stations[127][255] = {STATION_UNUSED, INADDR_ANY, in6addr_any, 0, ""};
	int totalStations = 0;

	/* Load !Stations file */
	int loadStations(void) {
		int result;
		unsigned char n, s;
		unsigned short p;
		char buffer[256];
		char ip[INET6_ADDRSTRLEN];
		char hash[FILESTORE_STATIONS_HASH_LENGTH];
		FILE *fp_stationsfile;

		printf("- Loading %s: ", STATIONSFILE);

		/* The console is the station with network ID 0 and station ID 0 */
		stations::stations[0][0].type = STATION_CONSOLE;

		fp_stationsfile = fopen(STATIONSFILE, "r");
		if (fp_stationsfile != NULL) {
			while (!feof(fp_stationsfile)) {
				if (fgets(buffer, sizeof(buffer), fp_stationsfile) != NULL) {
					if (buffer[0] != '#') {
						/* TODO: there's no size checking when fscanf-ing the values into users[]. If a string in the !Users file is larger than the size of the variables in users[], a buffer overvlow will happen */
						result = sscanf(buffer, "%hhu %hhu %s %hu %128s", &n, &s, ip, &p, hash);
						if ((result == 4) || (result == 5)) {
							if (n > 127) {
								fprintf(stderr, "Invalid econet network value: %i\n", n);
								continue;
							}
							if ((s < 1) || (s > 254)) {
								fprintf(stderr, "Invalid econet station value: %i\n", s);
								continue;
							}
							if (inet_pton(AF_INET, ip, &stations[n][s].ipv4) == 0) {
								if (inet_pton(AF_INET6, ip, &stations[n][s].ipv6) == 0) {
									fprintf(stderr, "Invalid IP address: %s\n", ip);
									continue;
								} else {
									stations[n][s].type = STATION_IPV6;
								}
							} else {
								stations[n][s].type = STATION_IPV4;
							}
							if (p == 0) {
								fprintf(stderr, "Invalid port number: %i\n", p);
								stations[n][s].type = STATION_UNUSED;
								continue;
							}
							if (result == 5) {
								if (strlen(hash) != (FILESTORE_STATIONS_HASH_LENGTH - 1)) {
									fprintf(stderr, "Invalid fingerprint size: %s\n", hash);
									stations[n][s].type = STATION_UNUSED;
									continue;
								}
								/* Validate that ascii contains a valid hex string */
								if (hash[strspn(hash, "0123456789abcdefABCDEF")] != 0) {
									fprintf(stderr, "Invalid characters in fingerprint: %s\n", hash);
									stations[n][s].type = STATION_UNUSED;
									continue;
								}
								strcpy(stations[n][s].fingerprint, hash);
							}
							stations[n][s].port = p;
//							printf("%i:%i IPv4=%08X IPv6=%X port=%i hash=%s\n", n, s, stations[n][s].ipv4, stations[n][s].ipv6, stations[n][s].port, hash);
							stations::totalStations++;
						}
					}
				}
			}
			fclose(fp_stationsfile);
			printf(" %i stations loaded.\n", stations::totalStations);
		} else {
			return(0x000000D6);
		}
		return(0);
	}
}
