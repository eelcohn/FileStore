/* none.h
 * Low-level driver for empty interface (none).
 *
 * (c) Eelco Huininga 2018
 */

#ifndef FILESTORE_ADAPTER_NONE_HEADER
#define FILESTORE_ADAPTER_NONE_HEADER

#include "../../econet.h"			// Included for Econet::frame

namespace api {
	int	initializeHardware(void);
	int	resetHardware(void);
	int	shutdownHardware(void);
	int	receiveData(econet::Frame *frame);
	int	transmitData(econet::Frame *frame, unsigned int length);
	bool	networkState(void);
	int	getClockSpeed(void);
	int	startClock(void);
	int	stopClock(void);
}

#endif

