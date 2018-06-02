/* api.h
 * Econet API, based upon API calls in Acorn ANFS and RiscOS
 *
 * (c) Eelco Huininga 2017
 */

#ifndef ECONET_API_HEADER
#define ECONET_API_HEADER

#include "econet.h"			// Included for Frame

namespace api {
	int	initializeHardware(void);
	int	resetHardware(void);
	void	startClock(void);
	void	stopClock(void);
	int	transmitData(econet::Frame *frame, unsigned int length);
	int	receiveData(econet::Frame *frame);
	bool	networkState(void);
}

#endif

