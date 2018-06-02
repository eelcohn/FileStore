/* star_time.cpp
 * *TIME command handler
 *
 * (c) Eelco Huininga 2017
 */

#include <cstdio>
#include <cstring>
#include <ctime>
#include "star_time.h"

using namespace std;



namespace commands {
	int star_time(char **args) {
		char buffer[128];
		time_t rawtime;
		struct tm* timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(buffer, BUFFER_LENGTH, "Good afternoon! It's %H:%M:%S\n", timeinfo);

		puts(buffer);
		return(0);
	}
};


