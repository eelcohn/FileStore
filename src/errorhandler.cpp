/* errorhandler.cpp
 * Econet file server - error handler
 *
 * (c) Eelco Huininga 2017-2019
 */

#include <cstdio>		// Included for printf() and sprintf()
#include "config.h"		// uint8_t
#include "errorhandler.h"	// typedef Error
#include "settings.h"		// Global configuration variables are defined here

Error errorMessages[] = {
	{0x00000021,	"Cannot find password file"},
	{0x00000022,	"Could not load !Stations file"},
	{0x00000023,	"Could not save !Stations file"},
	{0x00000024,	"Could not load !Users file"},
	{0x00000025,	"Could not save !Users file"},
	{0x00000030,	"Passwords don't match"},
	{0x00000083,	"Too much data"},
	{0x000000A8,	"Broken directory"},
	{0x000000AE,	"Not logged on"},
	{0x000000AF,	"Types don't match"},
	{0x000000B0,	"Bad rename"},
	{0x000000B1,	"Bad copy"},
	{0x000000B3,	"Directory full"},
	{0x000000B4,	"Directory not empty"},
	{0x000000B9,	"Bad password"},
	{0x000000BA,	"Insufficient privilege"},
	{0x000000BB,	"Wrong password"},
	{0x000000BC,	"User not known"},
	{0x000000BD,	"Access violation"},
	{0x000000BF,	"Who are you?"},
	{0x000000C0,	"Too many files open"},
	{0x000000C1,	"Not open for update"},
	{0x000000C2,	"Already open"},
	{0x000000C3,	"Object locked"},
	{0x000000C4,	"Already exists"},
	{0x000000C6,	"Disc full"},
	{0x000000C7,	"Disc fault"},
	{0x000000C8,	"Disc changed"},
	{0x000000C9,	"Disc write protected"},
	{0x000000CC,	"Bad filename"},
	{0x000000CF,	"Bad attribute"},
	{0x000000D6,	"File not found"},
	{0x000000DE,	"Channel"},
	{0x000000DF,	"EOF"},
	{0x000000FD,	"Bad string"},
	{0x000000FE,	"Bad command"},
	{0x00000311,	"No Econet hardware"},
	{0x000003A0,	"Line jammed"},
	{0x000003A1,	"Network error"},
	{0x000003A2,	"Not listening"},
	{0x000003A3,	"No clock signal detected on the Econet network"},
	{0x000003A5,	"No reply"},

	{0xC0000001,	"tokenizeCommandLine: allocation error"},
	{0xC0000002,	"tokenizeCommandLine: re-allocation error"},
	{0xC0000003,	"Can't catch SIGHUP"},
	{0xC0000004,	"Can't catch SIGINT"},
	{0xC0000005,	"Can't catch SIGTERM"},

	{0xC3140001,	"PiGPIO: Could not initialize hardware"},
	{0xC3140002,	"Could not reset hardware"}
};

using namespace std;

void errorHandler(uint32_t errorNumber) {
	unsigned int i;

	for (i = 0; i < 38; i++) {
		if (errorMessages[i].errno == errorNumber) {
			fprintf(stderr, (char *)settings::onError, "Warning", errorNumber, errorMessages[i].error);
			return;
		}
	}
	fprintf(stderr, (char *)settings::onError, "Warning", errorNumber, "** Unknown error **");
}

char * getErrorString(uint32_t errorNumber) {
	unsigned int i;

	for (i = 0; i < 38; i++) {
		if (errorMessages[i].errno == errorNumber) {
			return (char *)errorMessages[i].error;
		}
	}
	fprintf(stderr, (char *)settings::onError, "Warning", errorNumber, "** Unknown error **");
	return 0;
}

