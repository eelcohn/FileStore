/* debug.h
 * Debug and logging options
 *
 * (c) Eelco Huininga 2017
 */

#include <cstdio>			// Included for printf, FILE, fopen, fclose
#include "config.h"
#include "configuration.h"		// Included for global configuration

namespace debug {
	FILE *fp_networkLog;

	void networkLogStart(void) {
//		fp_networkLog = fopen(configure::networkLogFile, "w+");
		fp_networkLog = fopen("network.log", "w+");
		printf("i/f Di  Src   Dst  Ct Pt  Data                     ASCII   \n");
		printf("       Ne:St Ne:St                                         \n");
		printf("-----------------------------------------------------------\n");
	}

	void networkLog(void) {
		printf("eco Rx 00:D4 FF:FF 80 D0  2A 44 45 4C 45 54 45 00  *DELETE.\n");
	}

	void networkLogStop(void) {
		fclose(fp_networkLog);
	}
}

#ifdef DEBUGBUILD
	// Temporary include for debugging purposes (D00, D01 ... D80 etc)
#include <cstdlib>			// Included for strtol() and exit()
#include <cstring>			// Included for strcmp()
#include <pigpio.h>
#include "gpio.h"			// Included for ADLC_* definitions

namespace debug {
	int d(char **args) {
		int dataline, value, gpio_pin;
		char *end;

		dataline = strtol(args[1], &end, 10);
		value = strtol(args[2], &end, 10);

		if ((dataline < 0) || (dataline > 7))
			return 0x000000FE;

		if ((value != 0) && (value != 1))
			return 0x000000FE;

		switch (dataline) {
			case 0 :
				gpio_pin = ADLC_D0;
				break;

			case 1 :
				gpio_pin = ADLC_D1;
				break;

			case 2 :
				gpio_pin = ADLC_D2;
				break;

			case 3 :
				gpio_pin = ADLC_D3;
				break;

			case 4 :
				gpio_pin = ADLC_D4;
				break;

			case 5 :
				gpio_pin = ADLC_D5;
				break;

			case 6 :
				gpio_pin = ADLC_D6;
				break;

			case 7 :
				gpio_pin = ADLC_D7;
				break;

			default :
				printf("debug::d defaulted on switch! This should never happen!\n");
				exit(0x000000FE);
		}

printf("gpio=%i value=%i\n", gpio_pin, value);

		gpioSetMode(gpio_pin, PI_OUTPUT);
		if (value == 1)
			digitalWrite(gpio_pin, PI_HIGH);
		else
			digitalWrite(gpio_pin, PI_LOW);

		return(0);
	}

	int rs(char **args) {
		int reg;
		char *end;

		reg = strtol(args[1], &end, 10);

		if ((reg < 0) || (reg > 3))
			return 0x000000FE;
gpioSetMode(ADLC_A0, PI_OUTPUT);
gpioSetMode(ADLC_A1, PI_OUTPUT);
		if (reg & 0x01)
			gpioWrite(ADLC_A0, PI_HIGH);
		else
			gpioWrite(ADLC_A0, PI_LOW);
		if (reg & 0x02)
			gpioWrite(ADLC_A1, PI_HIGH);
		else
			gpioWrite(ADLC_A1, PI_LOW);

		return(0);
	}

	int rw(char **args) {
		gpioSetMode(ADLC_RW, PI_OUTPUT);
		if (strcmp(args[1], "R") == 0)
			gpioWrite(ADLC_RW, PI_HIGH);
		else if (strcmp(args[1], "W") == 0)
			gpioWrite(ADLC_RW, PI_LOW);
		else
			return 0x000000FE;

		return(0);
	}

	int cs(char **args) {
		gpioSetMode(ADLC_CS, PI_OUTPUT);
		if (strcmp(args[1], "ON") == 0)
			gpioWrite(ADLC_CS, PI_HIGH);
		else if (strcmp(args[1], "OFF") == 0)
			gpioWrite(ADLC_RW, PI_LOW);
		else
			return 0x000000FE;

		return(0);
	}

	int rst(char **args) {

		gpioSetMode(ADLC_RST, PI_OUTPUT);
		if (strcmp(args[1], "ON") == 0)
			gpioWrite(ADLC_RST, PI_HIGH);
		else if (strcmp(args[1], "OFF") == 0)
			gpioWrite(ADLC_RST, PI_LOW);
		else
			return 0x000000FE;

		return(0);
	}

	int phi(char **args) {
		gpioSetMode(ADLC_PHI2, PI_OUTPUT);
		if (strcmp(args[1], "ON") == 0)
			gpioWrite(ADLC_PHI2, PI_HIGH);
		else if (strcmp(args[1], "OFF") == 0)
			gpioWrite(ADLC_PHI2, PI_LOW);
		else
			return 0x000000FE;

		return(0);
	}
}
#endif

