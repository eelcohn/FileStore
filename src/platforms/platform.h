/* platform.h
 * Platform independant code for the Econet file server
 *
 * (c) Eelco Huininga 2017
 */

#include "../config.h"

#ifdef __GNUC__
#include "linux/linux.h"
#include "linux/limits.h"				/* PATH_MAX */
#endif
#if (FILESTORE_ADAPTER == FILESTORE_ADAPTER_NONE)
#include "adapters/none.h"
#endif
#if (FILESTORE_ADAPTER == FILESTORE_ADAPTER_REMA)
#include "adapters/rpi-gpio.h"
#endif

