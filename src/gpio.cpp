/* gpio.c
 * Low-level driver for the Econet module interface,
 * connected to the Raspberry Pi GPIO
 *
 * (c) Eelco Huininga 2017
 */

#include "gpio.h"
#include "wiringPi/wiringPi/wiringPi.h"

using namespace std;




namespace gpio {
	unsigned int clockSpeed;
	unsigned char dutyCycle;
	bool clockStarted;



	int initializeGPIO(void) {
#ifndef ECONET_DEBUGBUILD
		wiringPiSetupGpio();                  // Setup using the BCM GPIO pin numbers
#endif
		return (0);
	}

	int resetADLC(void) {
#ifdef ECONET_DEBUGBUILD
		return(0);
#endif
		// Set all pins to their default state
		pinMode(ADLC_D0, INPUT);
		pinMode(ADLC_D1, INPUT);
		pinMode(ADLC_D2, INPUT);
		pinMode(ADLC_D3, INPUT);
		pinMode(ADLC_D4, INPUT);
		pinMode(ADLC_D5, INPUT);
		pinMode(ADLC_D6, INPUT);
		pinMode(ADLC_D7, INPUT);
		pinMode(ADLC_A0, OUTPUT);
		pinMode(ADLC_A1, OUTPUT);
		pinMode(ADLC_CS, OUTPUT);
		pinMode(ADLC_RW, OUTPUT);
		pinMode(ADLC_RST, OUTPUT);
		pinMode(ADLC_IRQ, INPUT);

		pinMode(CLKIN, INPUT);
		pinMode(CLKIN_EN, OUTPUT);
		pinMode(CLKOUT, OUTPUT);
		pinMode(CLKOUT_EN, OUTPUT);

		// Initialize bus signals
		digitalWrite (ADLC_RST, HIGH);        // Don't reset the beast just yet
		digitalWrite (ADLC_CS, HIGH);         // Disable ADLC for now
		digitalWrite (ADLC_RW, HIGH);         // Read
		digitalWrite (ADLC_A0, LOW);          // Select register 0
		digitalWrite (ADLC_A1, LOW);          // 

		digitalWrite (CLKIN_EN, LOW);
		digitalWrite (CLKOUT, LOW);
		digitalWrite (CLKOUT_EN, LOW);

		// Start Phi2 clock
		pinMode(ADLC_PHI2, PWM_OUTPUT);
		pwmSetMode(PWM_MODE_MS);              // use a fixed frequency
		pwmSetRange(128);                     // range is 0-128
		pwmSetClock(15);                      // gives a precise 10kHz signal
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

	// Initialize ADLC registers
	void initializeADLC(void) {
		gpio::writeRegister(0, 0xC1);
		gpio::writeRegister(3, 0x1E);
		gpio::writeRegister(1, 0x00);
		gpio::writeRegister(0, 0x82);
		gpio::writeRegister(1, 0x67);
	}

	// Wait until the ADLC generates an IRQ
	void waitForADLCInterrupt(void) {
		bool interrupt;

		while (interrupt == false)
			interrupt = (gpio::readRegister(0) && 0x80);
	}

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
		pwmSetMode(PWM_MODE_MS);              // use a fixed frequency
		pwmSetRange(128);                     // range is 0-128
		pwmSetClock(15);                      // gives a precise 10kHz signal
		pwmWrite(CLKOUT, gpio::dutyCycle);      // duty cycle (0...128)
		digitalWrite (CLKOUT_EN, HIGH);
	}

	void stopClock(void) {
		digitalWrite (CLKOUT_EN, LOW);
	}
}
