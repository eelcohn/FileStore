/* gpio.c
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
		return (gpioInitialise());	// Initialize pigpio
	}

	int resetADLC(void) {
		// Set all pins to their default state
		gpioSetMode(ADLC_D0, INPUT);
		gpioSetMode(ADLC_D1, INPUT);
		gpioSetMode(ADLC_D2, INPUT);
		gpioSetMode(ADLC_D3, INPUT);
		gpioSetMode(ADLC_D4, INPUT);
		gpioSetMode(ADLC_D5, INPUT);
		Mode(ADLC_D6, INPUT);
		Mode(ADLC_D7, INPUT);
		ode(ADLC_A0, OUTPUT);
		Mode(ADLC_A1, OUTPUT);
		Mode(ADLC_CS, OUTPUT);
		Mode(ADLC_RW, OUTPUT);
		Mode(ADLC_RST, OUTPUT);
		Mode(ADLC_IRQ, INPUT);

		Mode(CLKIN, INPUT);
		Mode(CLKIN_EN, OUTPUT);
		Mode(CLKOUT, OUTPUT);
		Mode(CLKOUT_EN, OUTPUT);

		// Initialize bus signals
		digitalWrite (ADLC_RST, HIGH);        // Don't reset the beast just yet
		digitalWrite (ADLC_CS, HIGH);         // Disable ADLC for now
		digitalWrite (ADLC_RW, HIGH);         // Read
		digitalWrite (ADLC_A0, LOW);          // Select register 0
		digitalWrite (ADLC_A1, LOW);          // 

		digitalWrite (CLKIN_EN, LOW);
		digitalWrite (CLKOUT, LOW);
		digitalWrite (CLKOUT_EN, LOW);

		// Start Phi2 clock (min=0.5us max=10us for a 68B54 according to the datasheet)
		pinMode(ADLC_PHI2, PWM_OUTPUT);
		pwmSetMode(0, PWM_MODE_MS);              // use a fixed frequency
		pwmSetRange(0, 10);                     // range is 0-128
		pwmSetClock(0, 9);                      // gives a precise 10kHz signal
		pwmWrite(ADLC_PHI2, 64);                   // duty cycle of 50% (64/128)

		// Set up IRQ handler
		wiringPiISR(ADLC_IRQ, INT_EDGE_FALLING, &gpio::irqHandler);

		// Pulse RST low to reset the ADLC
		digitalWrite (ADLC_RST, LOW);
		delay(ADLC_RESET_PULSEWIDTH);
		digitalWrite (ADLC_RST, HIGH);

		gpio::initializeADLC();
		return (0);
	}

	int powerDownADLC(void) {
		// Pulse RST low to reset the ADLC
		digitalWrite (ADLC_RST, LOW);
		delay(ADLC_RESET_PULSEWIDTH);
		digitalWrite (ADLC_RST, HIGH);

		// Set all pins to their default state
		pinMode(ADLC_D0, INPUT);
		pinMode(ADLC_D1, INPUT);
		pinMode(ADLC_D2, INPUT);
		pinMode(ADLC_D3, INPUT);
		pinMode(ADLC_D4, INPUT);
		pinMode(ADLC_D5, INPUT);
		pinMode(ADLC_D6, INPUT);
		pinMode(ADLC_D7, INPUT);
		pinMode(ADLC_A0, INPUT);
		pinMode(ADLC_A1, INPUT);
		pinMode(ADLC_CS, INPUT);
		pinMode(ADLC_RW, INPUT);
		pinMode(ADLC_RST, INPUT);
		pinMode(ADLC_IRQ, INPUT);

		pinMode(CLKIN, INPUT);
		pinMode(CLKIN_EN, INPUT);
		pinMode(CLKOUT, INPUT);
		pinMode(CLKOUT_EN, INPUT);

		// Stop Phi2 clock
		pinMode(ADLC_PHI2, INPUT);
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

		digitalWrite (ADLC_RW, HIGH);
		if (reg & 0x01)
			digitalWrite (ADLC_A0, HIGH);
		else
			digitalWrite (ADLC_A0, LOW);
		if (reg & 0x02)
			digitalWrite (ADLC_A1, HIGH);
		else
			digitalWrite (ADLC_A1, LOW);
		digitalWrite (ADLC_CS, LOW);
		delay(ADLC_BUS_SETTLE_TIME);

		if (digitalRead(ADLC_D0))
			result |= 0x01;
		if (digitalRead(ADLC_D1))
			result |= 0x02;
		if (digitalRead(ADLC_D2))
			result |= 0x04;
		if (digitalRead(ADLC_D3))
			result |= 0x08;
		if (digitalRead(ADLC_D4))
			result |= 0x10;
		if (digitalRead(ADLC_D5))
			result |= 0x20;
		if (digitalRead(ADLC_D6))
			result |= 0x40;
		if (digitalRead(ADLC_D7))
			result |= 0x80;
		digitalWrite (ADLC_CS, HIGH);
		delay(ADLC_BUS_SETTLE_TIME);

		return (result);
	}

	void writeRegister(unsigned char reg, unsigned char value) {
		digitalWrite (ADLC_RW, LOW);

		// Set address bus
		if (reg & 0x01)
			digitalWrite (ADLC_A0, HIGH);
		else
			digitalWrite (ADLC_A0, LOW);
		if (reg & 0x02)
			digitalWrite (ADLC_A1, HIGH);
		else
			digitalWrite (ADLC_A1, LOW);

		// Set databus
		if (value & 0x01)
			digitalWrite(ADLC_D0, HIGH);
		else
			digitalWrite(ADLC_D0, LOW);
		if (value & 0x02)
			digitalWrite(ADLC_D1, HIGH);
		else
			digitalWrite(ADLC_D1, LOW);
		if (value & 0x04)
			digitalWrite(ADLC_D2, HIGH);
		else
			digitalWrite(ADLC_D2, LOW);
		if (value & 0x08)
			digitalWrite(ADLC_D3, HIGH);
		else
			digitalWrite(ADLC_D3, LOW);
		if (value & 0x10)
			digitalWrite(ADLC_D4, HIGH);
		else
			digitalWrite(ADLC_D4, LOW);
		if (value & 0x20)
			digitalWrite(ADLC_D5, HIGH);
		else
			digitalWrite(ADLC_D5, LOW);
		if (value & 0x40)
			digitalWrite(ADLC_D6, HIGH);
		else
			digitalWrite(ADLC_D6, LOW);
		if (value & 0x80)
			digitalWrite(ADLC_D7, HIGH);
		else
			digitalWrite(ADLC_D7, LOW);

		digitalWrite (ADLC_CS, LOW);
		delay(ADLC_BUS_SETTLE_TIME);
		digitalWrite (ADLC_CS, HIGH);
		delay(ADLC_BUS_SETTLE_TIME);
	}

	int setClockSpeed(unsigned int clockSpeed, unsigned char dutyCycle) {
		if (clockSpeed > 500000)
			return (-1);
		else
			gpio::clockSpeed = clockSpeed;

		if (dutyCycle > 128)
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
		pinMode(CLKOUT, PWM_OUTPUT);
		pwmSetMode(1, PWM_MODE_MS);              // use a fixed frequency
		pwmSetRange(1, 128);                     // range is 0-128
		pwmSetClock(1, 2);                      // gives a precise 10kHz signal
		pwmWrite(CLKOUT, gpio::dutyCycle);      // duty cycle (0...128)
		digitalWrite (CLKOUT_EN, HIGH);
	}

	void stopClock(void) {
		digitalWrite (CLKOUT_EN, LOW);
	}
}

