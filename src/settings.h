/* settings.h
 * Global configuration variables are defined here
 *
 * (c) Eelco Huininga 2017
 */

#ifndef ECONET_CONFIGURATION_HEADER
#define ECONET_CONFIGURATION_HEADER

#include "users.h"					// MAX_USER_FLAGS

namespace settings {
	extern unsigned char	*servername;
	extern unsigned char	*printername;
	extern unsigned char	econet_station;
	extern unsigned char	econet_network;
	extern unsigned char	aun_station;
	extern unsigned char	aun_network;
	extern unsigned short	aun_port;
	extern unsigned short	dtls_port;
	extern unsigned char	autolearn;
	extern bool		relay_only_known_networks;
	extern unsigned char	*volume;
	extern unsigned char	*printqueue;
	extern bool		clock;
	extern unsigned int	clockspeed;
	extern unsigned short	dutycycle;
	extern unsigned int	protectionLevel;	// Econet protection level of this file server
	extern unsigned char	*networkLogFile;
	extern unsigned char	onError[256];		// ON ERROR string
	extern unsigned char	defaultflags[MAX_USER_FLAGS];
}

#endif

