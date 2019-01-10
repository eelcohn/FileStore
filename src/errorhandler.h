/* errorhandler.h
 * Econet file server - error handler
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_ERRORHANDLER_HEADER
#define ECONET_ERRORHANDLER_HEADER

#include "config.h"		// uint8_t

#define ERR_LOCKED		0x000000C3
#define ERR_BADDIR		0x000000CE
#define ERR_BADATTRIBUTE	0x000000CF
#define ERR_FILENOTFOUND	0x000000D6
#define ERR_CHANNEL		0x000000DE
#define ERR_EOF			0x000000DF
#define ERR_BAD_COMMAND		0x000000FE

typedef struct {
	uint32_t	errno;
	const char	*error;
} Error;

extern Error errorMessages[];

void errorHandler(uint32_t errorNumber);
char * getErrorString(uint32_t errorNumber);

#endif

