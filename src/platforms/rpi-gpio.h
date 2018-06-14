/* rpi-gpio.h
 * Low-level driver for the Econet module interface,
 * connected to the Raspberry Pi GPIO
 *
 * (c) Eelco Huininga 2017-2018
 */

#ifndef ECONET_GPIO_HEADER
#define ECONET_GPIO_HEADER

#include "../econet.h"			// Included for Econet::frame
#include <pigpio.h>

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

#define ADLC_RESET_PULSEWIDTH	500000	// Pulse RESET low for 500ms (minimum RESET pulse with for the 68B54 according to the datasheet is 0.40us)
#define ADLC_BUS_SETTLE_TIME	1000	// Allow 1 ms for reads and writes to the ADLC
#define ADLC_INTERRUPT_TIMEOUT	10	// 10ms before an pigpio interrupt times out

#define MAX_CLOCKSPEED	1000000		// Maximum allowed Econet clock speed is 1MHz
#define MAX_DUTYCYCLE	999999		// Maximum allowed duty cycle is 999999/1000000

namespace api {
	int	initializeHardware(void);
	int	resetHardware(void);
	int	shutdownHardware(void);
	int	transmitData(econet::Frame *frame, unsigned int length);
	int	receiveData(econet::Frame *frame);
	bool	networkState(void);
	int	setClockSpeed(unsigned int clockSpeed, unsigned int dutyCycle);
	int	getClockSpeed(void);
	void	startClock(void);
	void	stopClock(void);
}

namespace rpi_gpio {
	int	resetADLC(void);
	void	initializeADLC(void);
	void	waitForADLCInterrupt(void);
	void	irqHandler(void);
	unsigned char readRegister(unsigned char reg);
	void	writeRegister(unsigned char reg, unsigned char value);
	int	transmitData(econet::Frame *frame, unsigned int length);	
	int	receiveData(econet::Frame *frame);
	bool	networkState(void);
	int	setClockSpeed(unsigned int clockSpeed, unsigned int dutyCycle);
	int	getClockSpeed(void);
	int	getClockSpeed_inthandler(void);
	void	startClock(void);
	void	stopClock(void);
}
#endif

