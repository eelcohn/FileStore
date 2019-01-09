/* none.h
 * Low-level driver for empty interface (none).
 *
 * (c) Eelco Huininga 2018
 */

#include "none.h"

using namespace std;




namespace api {
	int initializeHardware(void) {
		return (0);
	}

	int resetHardware(void) {
		return (0);
	}

	int shutdownHardware(void) {
		return (0);
	}

	int receiveData(__attribute__((__unused__))econet::Frame *frame) {
		return (0);
	}
 
	int transmitData(__attribute__((__unused__))econet::Frame *frame, __attribute__((__unused__))unsigned int length) {
		return (0);
	}

	bool networkState(void) {
		return (false);
	}

	int getClockSpeed(void) {
		return (0);
	}

	int startClock(void) {
		return(0x00000311);
	}

	int stopClock(void) {
		return(0x00000311);
	}
}

