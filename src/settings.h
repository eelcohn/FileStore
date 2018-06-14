/* settings.h
 * Global configuration variables are defined here
 *
 * (c) Eelco Huininga 2017
 */

#ifndef ECONET_CONFIGURATION_HEADER
#define ECONET_CONFIGURATION_HEADER

namespace configuration {
	extern unsigned char	*servername;
	extern unsigned char	*printername;
	extern unsigned char	econet_station;
	extern unsigned char	econet_network;
	extern unsigned char	ethernet_station;
	extern unsigned char	ethernet_network;
	extern unsigned int	autolearn;
	extern bool		relay_only_known_networks;
	extern unsigned char	*volume;
	extern unsigned char	*printqueue;
	extern bool		clock;
	extern unsigned int	clockspeed;
	extern unsigned short	dutycycle;
	extern unsigned int	protectionLevel;	// Econet protection level of this file server
	extern unsigned char	*networkLogFile;
	extern char		onError[256];		// ON ERROR string
};

#endif

