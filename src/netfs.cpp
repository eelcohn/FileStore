/* netfs.cpp
 * Handler for all NetFS commands
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>		// ptinf()
#include "platforms/platform.h"

using namespace std;



namespace commands {
	int access(char **args) {
		return(0);
	}

	int cat(char **args) {
		return(0);
	}

	int cdir(char **args) {
		return(0);
	}

	int del(char **args) {
		return(0);
	}

	int i_am(char **args) {
		return(0);
	}

	int clockspeed(char **args) {
		printf("Measured clockspeed is %i Hz\n", api::getClockSpeed());
		return(0);
	}
}


