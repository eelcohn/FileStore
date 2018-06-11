/* gpio.h * Low-level driver for the Econet module interface,
 * connected to the Raspberry Pi GPIO
 *
 * (c) Eelco Huininga 2017
 */

#ifndef ECONET_GPIO_HEADER
#define ECONET_GPIO_HEADER

/* Disables setup of WiringPi: remove on final build */
#define ECONET_DEBUGBUILD 1

#define ADLC_D0		17		//
#define ADLC_D1		18		//
#define ADLC_D2		27		//
#define ADLC_D3		22		//
#define ADLC_D4		23		//
#define ADLC_D5		24		//
#define ADLC_D6		25		//
#define ADLC_D7		4		//
#define ADLC_A0		2		//
#define ADLC_A1		3		//
#define ADLC_CS		10		//
#define ADLC_RW		9		//
#define ADLC_RST	8		//
#define ADLC_IRQ	7		//
#define ADLC_PHI2	12		//
#define CLKOUT		13		//
#define CLKOUT_EN	6		//
#define CLKIN		20		//
#define CLKIN_EN	16		//

#define ADLC_RESET_PULSEWIDTH 10       // Pulse RESET low for 500ms (minimum RESET pulse with for the 68B54 according to the datasheet is 0.40us)
#define ADLC_BUS_SETTLE_TIME 1         // Allow 1 ms for reads and writes to the ADLC

namespace gpio {
	int initializeGPIO(void);
	int resetADLC(void);
	void initializeADLC(void);
	void waitForADLCInterrupt(void);
	void irqHandler(void);
	unsigned char readRegister(unsigned char reg);
	void writeRegister(unsigned char reg, unsigned char value);
	int setClockSpeed(unsigned int clockSpeed, unsigned char dutyCycle);
	int getClockSpeed(void);
	void startClock(void);
	void stopClock(void);
}
#endif

