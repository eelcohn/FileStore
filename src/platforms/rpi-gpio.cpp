/* rpi-gpio.cpp
 * Low-level driver for the Econet module interface,
 * connected to the Raspberry Pi GPIO
 *
 * (c) Eelco Huininga 2017-2018
 */

#include "rpi-gpio.h"
#include "../settings.h"

using namespace std;




namespace api {
	bool clockStarted;



	int initializeHardware(void) {
		return (gpioInitialise());	// Initialize the pigpio library
	}

	int resetHardware(void) {
		return (rpi_gpio::resetADLC());		// Reset the Econet Adapter
	}

	int shutdownHardware(void) {
		gpioTerminate();		// Terminate the pigpio library
		return (0);
	}

	int receiveData(econet::Frame *frame) {
		return rpi_gpio::receiveData(frame);
	}
 
	int transmitData(econet::Frame *frame, unsigned int length) {
		return rpi_gpio::transmitData(frame, length);
	}

	bool networkState(void) {
		return (rpi_gpio::networkState());
	}

	int setClockSpeed(unsigned int clockSpeed, unsigned int dutyCycle) {
		return(rpi_gpio::setClockSpeed(clockSpeed, dutyCycle));
	}

	int getClockSpeed(void) {
		return(rpi_gpio::getClockSpeed());
	}

	void startClock(void) {
		rpi_gpio::startClock();
	}

	// Stop Econet clock
	void stopClock(void) {
		rpi_gpio::stopClock();
	}
}

namespace rpi_gpio {
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
//		gpioSetISRFunc(ADLC_IRQ, FALLING_EDGE, ADLC_INTERRUPT_TIMEOUT, &gpio::irqHandler);

		// Pulse RST low to reset the ADLC
		gpioWrite(ADLC_RST, PI_LOW);
		gpioSleep(PI_TIME_RELATIVE, 0, ADLC_RESET_PULSEWIDTH);
		gpioWrite(ADLC_RST, PI_HIGH);

		rpi_gpio::initializeADLC();
		return (0);
	}

	int powerDownADLC(void) {
		// Pulse RST low to reset the ADLC
		gpioWrite(ADLC_RST, PI_LOW);
		gpioSleep(PI_TIME_RELATIVE, 0, ADLC_RESET_PULSEWIDTH);
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
		rpi_gpio::writeRegister(0, 0xC1);
		rpi_gpio::writeRegister(3, 0x1E);
		rpi_gpio::writeRegister(1, 0x00);
		rpi_gpio::writeRegister(0, 0x82);
		rpi_gpio::writeRegister(1, 0x67);
	}

	// Poll the ADLC until an IRQ occurs
	void waitForADLCInterrupt(void) {
		bool interrupt;

		interrupt = false;
		while (interrupt == false)
			interrupt = (rpi_gpio::readRegister(0) && 0x80);
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
		gpioSleep(PI_TIME_RELATIVE, 0, ADLC_BUS_SETTLE_TIME);

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
		gpioSleep(PI_TIME_RELATIVE, 0, ADLC_BUS_SETTLE_TIME);

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
			gpioWrite(ADLC_D0, PI_HIGH);
		else
			gpioWrite(ADLC_D0, PI_LOW);
		if (value & 0x02)
			gpioWrite(ADLC_D1, PI_HIGH);
		else
			gpioWrite(ADLC_D1, PI_LOW);
		if (value & 0x04)
			gpioWrite(ADLC_D2, PI_HIGH);
		else
			gpioWrite(ADLC_D2, PI_LOW);
		if (value & 0x08)
			gpioWrite(ADLC_D3, PI_HIGH);
		else
			gpioWrite(ADLC_D3, PI_LOW);
		if (value & 0x10)
			gpioWrite(ADLC_D4, PI_HIGH);
		else
			gpioWrite(ADLC_D4, PI_LOW);
		if (value & 0x20)
			gpioWrite(ADLC_D5, PI_HIGH);
		else
			gpioWrite(ADLC_D5, PI_LOW);
		if (value & 0x40)
			gpioWrite(ADLC_D6, PI_HIGH);
		else
			gpioWrite(ADLC_D6, PI_LOW);
		if (value & 0x80)
			gpioWrite(ADLC_D7, PI_HIGH);
		else
			gpioWrite(ADLC_D7, PI_LOW);

		gpioWrite(ADLC_CS, PI_LOW);
		gpioSleep(PI_TIME_RELATIVE, 0, ADLC_BUS_SETTLE_TIME);
		gpioWrite(ADLC_CS, PI_HIGH);
		gpioSleep(PI_TIME_RELATIVE, 0, ADLC_BUS_SETTLE_TIME);
	}

	// Send an Econet packet over the Econet interface
	int transmitData(econet::Frame *frame, unsigned int length) {
		unsigned int i = 0;

		rpi_gpio::writeRegister(1, 0xE7);
		rpi_gpio::writeRegister(0, 0x44);
		do {
			rpi_gpio::waitForADLCInterrupt();
//			rpi_gpio::writeRegister(2, (unsigned char *) &frame[i);
			i++;
		} while (i < length);
		rpi_gpio::writeRegister(1, 0x3F);
		rpi_gpio::waitForADLCInterrupt();

		return (0);
	}

	// Receive an Econet packet over the Econet interface
	int receiveData(econet::Frame *frame) {
		int result, i = 0;
		unsigned char status;

		frame->status = 0x00;

		rpi_gpio::writeRegister(0, 0x82);
		if (rpi_gpio::readRegister(1) && 0x01) {
			while (!(rpi_gpio::readRegister(1) && 0x80))
				frame->data[i++] = rpi_gpio::readRegister(2);

			rpi_gpio::writeRegister(0, 0x00);
			rpi_gpio::writeRegister(1, 0x84);
			status = rpi_gpio::readRegister(1);
			if (status && 0x02) {
				if (status && 0x80)
					status = rpi_gpio::readRegister(2);	// Fetch last byte in receive buffer
				if (frame->data[3] == 0)	// Replace local network (0) with our real network number
					frame->data[3] = configuration::econet_network;
				result = i - 1;
			} else
				return(-1);	// No valid frame found
		} else
			return(-2);
		return (result);
	}

	// SWI &4001A: Econet_NetworkState
	bool networkState(void) {
		// Return the DCD bit in ADLC status register 2
		return (rpi_gpio::readRegister(1) && 0x20);
	}

	int setClockSpeed(unsigned int clockSpeed, unsigned int dutyCycle) {
		if (clockSpeed > MAX_CLOCKSPEED)
			return (-1);
		else
			configuration::clockspeed = clockSpeed;

		if (dutyCycle > MAX_DUTYCYCLE)
			return (-2);
		else
			configuration::dutycycle = dutyCycle;

		if (api::clockStarted == true)
			rpi_gpio::startClock();

		return (0);
	}

	int getClockSpeed(void) {
		return (-1);
	}

	void startClock(void) {
		gpioHardwarePWM(CLKOUT, configuration::clockspeed, (configuration::dutycycle * 10000));
	}

	// Stop Econet clock
	void stopClock(void) {
		gpioHardwarePWM(CLKOUT, 0, 0);
	}
}

