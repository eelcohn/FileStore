/* stations.h
 * All Stations related functions
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_STATIONS_HEADER
#define ECONET_STATIONS_HEADER

#define FILESTORE_STATIONS_HASH_LENGTH (SHA512_DIGEST_LENGTH * 2) + 1

#include <netinet/in.h>			// Included for struct in_addr
#include <openssl/sha.h>		// Included for SHA256_DIGEST_LENGTH

enum STATION_TYPES {STATION_UNUSED, STATION_CONSOLE, STATION_ECONET, STATION_IPV4, STATION_IPV6};
extern const char *station_type[];


typedef struct {
	unsigned char	type;						// Station type
	in_addr		ipv4;						// IP address, or 0 if this station doesn't have an IP address
	in6_addr	ipv6;						// IPv6 address, or 0 if this station doesn't have an IPv6 address
	unsigned short	port;						// UDP port, or 0 if this station doesn't have an IP address
	char		fingerprint[FILESTORE_STATIONS_HASH_LENGTH];	// Fingerprint of the certificate used by this station, or empty when no DTLS is used
} Station;

namespace stations {
	extern Station stations[127][255];

	int loadStations(void);
}

#endif

