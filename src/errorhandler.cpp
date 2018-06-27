/* errorhandler.cpp
 * Econet file server - error handler
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>		// Included for printf() and sprintf()
#include "settings.h"		// Global configuration variables are defined here

using namespace std;

const char *errorMessages[][2] = {
	{"000000C9",	"Disc write protected"},
	{"000000CC",	"Bad filename"},
	{"000000D6",	"File not found"},
	{"000000DE",	"Channel"},
	{"000000FE",	"Bad command"},
	{"000003A0",	"Line jammed"},
	{"000003A1",	"Network error"},
	{"000003A2",	"Not listening"},
	{"000003A3",	"No clock signal detected on the Econet network"},
	{"000003A5",	"No reply"}
};

void errorHandler(int errorNumber, const char *errorMessage) {
	fprintf(stderr, (char *)settings::onError, "Warning", errorNumber, errorMessage);
}

