/* api.c
 * Econet API, based upon API calls in Acorn ANFS and RiscOS
 *
 * (c) Eelco Huininga 2017
 */

#include "econet.h"			// Included for Frame
#include "gpio.h"
#include "configuration.h"



namespace api {
	// Initialize the Econet specific hardware
	int initializeHardware(void) {
		return (gpio::initializeGPIO());
	}

	int resetHardware(void) {
		return (gpio::resetADLC());
	}

	void startClock(void) {
		gpio::startClock();
	}

	void stopClock(void) {
		gpio::stopClock();
	}

	// Send an Econet packet over the Econet interface
	int transmitData(econet::Frame *frame, unsigned int length) {
		unsigned int i = 0;

		gpio::writeRegister(1, 0xE7);
		gpio::writeRegister(0, 0x44);
		do {
			gpio::waitForADLCInterrupt();
//			gpio::writeRegister(2, (unsigned char *) &frame[i);
			i++;
		} while (i < length);
		gpio::writeRegister(1, 0x3F);
		gpio::waitForADLCInterrupt();

		return (0);
	}

	// Receive an Econet packet over the Econet interface
	int receiveData(econet::Frame *frame) {
		int result, i = 0;
		unsigned char status;

		frame->status = 0x00;

		gpio::writeRegister(0, 0x82);
		if (gpio::readRegister(1) && 0x01) {
			while (!(gpio::readRegister(1) && 0x80))
				frame->data[i++] = gpio::readRegister(2);

			gpio::writeRegister(0, 0x00);
			gpio::writeRegister(1, 0x84);
			status = gpio::readRegister(1);
			if (status && 0x02) {
				if (status && 0x80)
					status = gpio::readRegister(2);	// Fetch last byte in receive buffer
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
		return (gpio::readRegister(1) && 0x20);
	}
}

