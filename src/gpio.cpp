/* gpio.cpp
 * Low-level driver for the Econet module interface,
 * connected to the Raspberry Pi GPIO
 *
 * (c) Eelco Huininga 2017-2018
 */

#include "gpio.h"
#include <pigpio.h>

using namespace std;




namespace gpio {
	unsigned int clockSpeed;
	unsigned char dutyCycle;
	bool clockStarted;



	int initializeGPIO(void) {
		return (gpioInitialise());	// Initialize the pigpio library
	}

	int stopGPIO(void) {
		return (gpioTerminate());	// Terminate the pigpio library
	}

	int resetADLC(void) {
		// Set all pins to their default state
		gpioSetMode(ADLC_D0, PI_INPUT);
		gpioSetMode(ADLC_D1, PI_INPUT);
		gpioSetMode(ADLC_D2, PI_INPUT);
		gpioSetMode(ADLC_D3, PI_INPUT);
		gpioSetMode(ADLC_D4, PI_INPUT);
		gpioSetMode(ADLC_D5, PI_INPUT);
		gpioSetMode(ADLC_D6, PI_INPUT);
		gpioSetMode(ADLC_D7, PI_INPUT);
		gpioSetMode(ADLC_A0, PI_OUTPUT);
		gpioSetMode(ADLC_A1, PI_OUTPUT);
		gpioSetMode(ADLC_CS, PI_OUTPUT);
		gpioSetMode(ADLC_RW, PI_OUTPUT);
		gpioSetMode(ADLC_RST, PI_INPUT);
		gpioSetMode(ADLC_IRQ, PI_INPUT);

		gpioSetMode(CLKIN, PI_INPUT);
		gpioSetMode(CLKIN_EN, PI_OUTPUT);
		gpioSetMode(CLKOUT, PI_OUTPUT);
		gpioSetMode(CLKOUT_EN, PI_OUTPUT);

		// Initialize bus signals
		gpioWrite (ADLC_RST, PI_HIGH);			// Don't reset the beast just yet
		gpioWrite (ADLC_CS, PI_HIGH);			// Disable ADLC for now
		gpioWrite (ADLC_RW, PI_HIGH);			// Read
		gpioWrite (ADLC_A0, PI_LOW);			// Select register 0
		gpioWrite (ADLC_A1, PI_LOW);			// 

		gpioWrite (CLKIN_EN, PI_LOW);
		gpioWrite (CLKOUT, PI_LOW);
		gpioWrite (CLKOUT_EN, PI_LOW);

		// Start Phi2 clock (min=0.5us max=10us for a 68B54 according to the datasheet)
		gpioHardwarePWM(ADLC_PHI2, 1000000, 500000);	// Set phi2 to 1MHz, 50% duty cycle

		// Set up IRQ handler
		gpioSetISRFunc(ADLC_IRQ, FALLING_EDGE, ADLC_INTERRUPT_TIMEOUT, &gpio::irqHandler);

		// Pulse RST low to reset the ADLC
		gpioWrite(ADLC_RST, PI_LOW);
		delay(ADLC_RESET_PULSEWIDTH);
		gpioWrite(ADLC_RST, PI_HIGH);

		gpio::initializeADLC();
		return (0);
	}

	int powerDownADLC(void) {
		// Pulse RST low to reset the ADLC
		gpioWrite(ADLC_RST, PI_LOW);
		delay(ADLC_RESET_PULSEWIDTH);
		gpioWrite(ADLC_RST, PI_HIGH);

		// Stop Phi2 clock
		gpioHardwarePWM(ADLC_PHI2, 0, 0);

		// Set all pins to their default state
		gpioSetMode(ADLC_D0, PI_INPUT);
		gpioSetMode(ADLC_D1, PI_INPUT);
		gpioSetMode(ADLC_D2, PI_INPUT);
		gpioSetMode(ADLC_D3, PI_INPUT);
		gpioSetMode(ADLC_D4, PI_INPUT);
		gpioSetMode(ADLC_D5, PI_INPUT);
		gpioSetMode(ADLC_D6, PI_INPUT);
		gpioSetMode(ADLC_D7, PI_INPUT);
		gpioSetMode(ADLC_A0, PI_INPUT);
		gpioSetMode(ADLC_A1, PI_INPUT);
		gpioSetMode(ADLC_CS, PI_INPUT);
		gpioSetMode(ADLC_RW, PI_INPUT);
		gpioSetMode(ADLC_RST, PI_INPUT);
		gpioSetMode(ADLC_IRQ, PI_INPUT);

		gpioSetMode(CLKIN, PI_INPUT);
		gpioSetMode(CLKIN_EN, PI_INPUT);
		gpioSetMode(CLKOUT, PI_INPUT);
		gpioSetMode(CLKOUT_EN, PI_INPUT);

		return (0);
	}

	// Initialize ADLC registers
	void initializeADLC(void) {
		gpio::writeRegister(0, 0xC1);
		gpio::writeRegister(3, 0x1E);
		gpio::writeRegister(1, 0x00);
		gpio::writeRegister(0, 0x82);
		gpio::writeRegister(1, 0x67);
	}

	// Poll the ADLC until an IRQ occurs
	void waitForADLCInterrupt(void) {
		bool interrupt;

		interrupt = false;
		while (interrupt == false)
			interrupt = (gpio::readRegister(0) && 0x80);
	}

	// ADLC hardware IRQ handler
	void irqHandler(void) {
	}

	unsigned char readRegister(unsigned char reg) {
		unsigned char result = 0x00;

		gpioWrite(ADLC_RW, PI_HIGH);
		if (reg & 0x01)
			gpioWrite(ADLC_A0, PI_HIGH);
		else
			gpioWrite(ADLC_A0, PI_LOW);
		if (reg & 0x02)
			gpioWrite(ADLC_A1, PI_HIGH);
		else
			gpioWrite(ADLC_A1, PI_LOW);
		gpioWrite(ADLC_CS, PI_LOW);
		delay(ADLC_BUS_SETTLE_TIME);

		if (gpioRead(ADLC_D0))
			result |= 0x01;
		if (gpioRead(ADLC_D1))
			result |= 0x02;
		if (gpioRead(ADLC_D2))
			result |= 0x04;
		if (gpioRead(ADLC_D3))
			result |= 0x08;
		if (gpioRead(ADLC_D4))
			result |= 0x10;
		if (gpioRead(ADLC_D5))
			result |= 0x20;
		if (gpioRead(ADLC_D6))
			result |= 0x40;
		if (gpioRead(ADLC_D7))
			result |= 0x80;
		gpioWrite(ADLC_CS, PI_HIGH);
		delay(ADLC_BUS_SETTLE_TIME);

		return (result);
	}

	void writeRegister(unsigned char reg, unsigned char value) {
		gpioWrite(ADLC_RW, PI_LOW);

		// Set address bus
		if (reg & 0x01)
			gpioWrite(ADLC_A0, PI_HIGH);
		else
			gpioWrite(ADLC_A0, PI_LOW);
		if (reg & 0x02)
			gpioWrite(ADLC_A1, PI_HIGH);
		else
			gpioWrite(ADLC_A1, PI_LOW);

		// Set databus
		if (value & 0x01)
			digitalWrite(ADLC_D0, PI_HIGH);
		else
			digitalWrite(ADLC_D0, PI_LOW);
		if (value & 0x02)
			digitalWrite(ADLC_D1, PI_HIGH);
		else
			digitalWrite(ADLC_D1, PI_LOW);
		if (value & 0x04)
			digitalWrite(ADLC_D2, PI_HIGH);
		else
			digitalWrite(ADLC_D2, PI_LOW);
		if (value & 0x08)
			digitalWrite(ADLC_D3, PI_HIGH);
		else
			digitalWrite(ADLC_D3, PI_LOW);
		if (value & 0x10)
			digitalWrite(ADLC_D4, PI_HIGH);
		else
			digitalWrite(ADLC_D4, PI_LOW);
		if (value & 0x20)
			digitalWrite(ADLC_D5, PI_HIGH);
		else
			digitalWrite(ADLC_D5, PI_LOW);
		if (value & 0x40)
			digitalWrite(ADLC_D6, PI_HIGH);
		else
			digitalWrite(ADLC_D6, PI_LOW);
		if (value & 0x80)
			digitalWrite(ADLC_D7, PI_HIGH);
		else
			digitalWrite(ADLC_D7, PI_LOW);

		gpioWrite(ADLC_CS, PI_LOW);
		delay(ADLC_BUS_SETTLE_TIME);
		gpioWrite(ADLC_CS, PI_HIGH);
		delay(ADLC_BUS_SETTLE_TIME);
	}

	int setClockSpeed(unsigned int clockSpeed, unsigned int dutyCycle) {
		if (clockSpeed > MAX_CLOCKSPEED)
			return (-1);
		else
			gpio::clockSpeed = clockSpeed;

		if (dutyCycle > MAX_DUTYCYCLE)
			return (-2);
		else
			gpio::dutyCycle = dutyCycle;

		if (gpio::clockStarted == TRUE)
			gpio::startClock();

		return (0);
	}

	int getClockSpeed(void) {
		return (-1);
	}

	void startClock(void) {
		gpioHardwarePWM(CLKOUT, gpio::clockSpeed, gpio::dutyCycle);
	}

	// Stop Econet clock
	void stopClock(void) {
		gpioHardwarePWM(CLKOUT, 0, 0);
	}
}

