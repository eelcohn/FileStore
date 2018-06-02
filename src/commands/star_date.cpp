/* date.cpp
 * *DATE command handler
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstring>
#include <cstdio>
#include <ctime>
#include "star_date.h"

using namespace std;



namespace commands {
	int date(char **args) {
		char buffer[128];
		time_t rawtime;
		struct tm * timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime (buffer, BUFFER_LENGTH, "Today is %A the %dth of %B %Y\n", timeinfo);

		puts(buffer);
		return(0);
	}
}


