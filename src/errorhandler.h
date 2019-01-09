/* errorhandler.h
 * Econet file server - error handler
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_ERRORHANDLER_HEADER
#define ECONET_ERRORHANDLER_HEADER

#include "config.h"		// uint8_t

typedef struct {
	uint32_t	errno;
	const char	*error;
} Error;

extern Error errorMessages[];

void errorHandler(uint32_t errorNumber);
char * getErrorString(uint32_t errorNumber);

#endif

