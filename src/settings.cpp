/* settings.cpp
 * Defaults for global settings
 *
 * (c) Eelco Huininga 2017-2019
 */

#include <cstdio>			// NULL
#include "settings.h"
#include "users.h"					// MAX_USER_FLAGS

using namespace std;



namespace settings {
	unsigned char	*servername			= (unsigned char *)"PISTORE";
	unsigned char	*printername			= (unsigned char *)"PIPRN";
	unsigned char	econet_station			= 254;
	unsigned char	econet_network			= 1;
	unsigned char	aun_station;
	unsigned char	aun_network;
	unsigned short	aun_port			= 32768;
	unsigned short	dtls_port			= 33859;
	unsigned char	autolearn			= 0;					// Autolearning for !Stations file is OFF (1=SESSION: only for this session, do not update !Stations / 2=FULL: add new stations to !Stations file)
	bool		relay_only_known_networks	= false;
	unsigned char	*volume				= NULL;
	unsigned char	*printqueue			= (unsigned char *)"/usr/bin/lpr";
	bool		clock				= false;
	unsigned int	clockspeed			= 30000;
	unsigned short	dutycycle			= 25;
	unsigned int	protectionLevel;
	unsigned char	*networkLogFile			= (unsigned char *)"network.log";
	unsigned char	onError[256]			= "%s &%08X: %s\n";
	unsigned char	defaultflags[MAX_USER_FLAGS]	= "P";
}

