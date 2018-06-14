/* api.c
 * Econet API, based upon API calls in Acorn ANFS and RiscOS
 *
 * (c) Eelco Huininga 2017
 */

#include "econet.h"			// Included for Frame
#include "gpio.h"
#include "configuration.h"



namespace api {
	// Initialize the Econet specific hardware
	int initializeHardware(void) {
		return (gpio::initializeGPIO());
	}

	int resetHardware(void) {
		return (gpio::resetADLC());
	}

	void startClock(void) {
		gpio::startClock();
	}

	void stopClock(void) {
		gpio::stopClock();
	}
}

