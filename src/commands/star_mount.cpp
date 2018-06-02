/* mount.cpp
 * *MOUNT command handler
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>			// Included for NULL and fopen()
#include "../configuration.h"		// Global configuration variables are defined here

extern FILE	*fp_volume;

using namespace std;



namespace commands {
	int mount(char **args) {
		if ((fp_volume = fopen(args[1], "r")) != NULL) {
			configuration::volume = (unsigned char *)args[1];
		} else {
			fp_volume = NULL;
			return(0x000000D6);	// D6 File not found
		}			
		return(0);
	}
}


