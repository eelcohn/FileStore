/* stations.h
 * All Stations related functions
 *
 * (c) Eelco Huininga 2017-2018
 */

#ifndef ECONET_STATIONS_HEADER
#define ECONET_STATIONS_HEADER

#include <netinet/in.h>		// Included for struct in_addr



typedef struct {
	struct in_addr	addr;		// IP address, or 0 if this station doesn't have an IP address
	unsigned short	port;		// UDP port, or 0 if this station doesn't have an IP address
	unsigned int	user;		// User ID of the user currently logged in at this station
} Station;

namespace stations {
	extern Station stations[127][255];

	bool loadStations(void);
};
#endif

