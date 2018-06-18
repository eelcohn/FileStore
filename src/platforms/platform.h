/* platform.h
 * Platform independant code for the Econet file server
 *
 * (c) Eelco Huininga 2017
 */

#ifdef __GNUC__
#include "linux.h"
#endif
#ifndef ECONET_ECOIF
#include "rpi-gpio.h"
#endif

