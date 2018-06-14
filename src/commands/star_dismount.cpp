/* dismount.cpp
 * *DISMOUNT command handler
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>			// Included for NULL and fclose()
#include "../settings.h"		// Global configuration variables are defined here

extern FILE	*fp_volume;

using namespace std;



namespace commands {
	int dismount(char **args) {
		if(fp_volume != NULL) {
			fclose(fp_volume);
			fp_volume = NULL;
			configuration::volume = NULL;
		}
		return(0);
	}
}


