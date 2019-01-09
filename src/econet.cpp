/* econet.cpp
 * All Econet send and receive functions
 *
 * (c) Eelco Huininga 2017-2019
 */

#include <cstdlib>		// Included for strtol()
#include <cstring>		// Included for memcpy(), strlen()
#include <ctime>		// Included for time(), tm

#include "main.h"		// Included for bye variable
#include "errorhandler.h"	// errorMessages[]
#include "platforms/platform.h"	// All high-level API calls
#include "settings.h"		// Global configuration variables are defined here
#include "econet.h"		// Header file for this code
#include "aun.h"		// Included for aun::transmitFrame()
#include "cli.h"		// Included for commands::netmonPrintFrame()
#include "netfs.h"		// getDiscTitle()

using namespace std;



namespace econet {
	Station known_networks[256];
	Session sessions[ECONET_MAX_SESSIONS];
	ProtoHandlers protohandlers[256] = {
		NULL
//		[0x99] = netfs::protohandler
	};
	uint8_t	printer_status = 0, printer_network = 0, printer_station = 0;
	uint8_t	port99save_replyport = 0;
	FILE	*fp_printbuffer;
	bool	netmon;

	/* Periodically check if we've received an Econet network package */
	void pollNetworkReceive(void) {
		int rx_length;
		econet::Frame frame;

		while (bye == false) {
			rx_length = api::receiveData(&frame);
			if ((rx_length > 0) && (econet::netmon))
				netmonPrintFrame("eco  ", false, &frame, rx_length);
			if (econet::validateFrame((econet::Frame *) &frame, rx_length)) {
				if (frame.flags || ECONET_FRAME_TOLOCAL) {
					/* Frame is addressed to a station on our local network */
					if (frame.flags || ECONET_FRAME_TOME) {
						/* Frame is addressed to us */
						econet::processFrame(&frame, rx_length);
					} else {
						/* Frame is addressed to a station on our local network */
						frame.rawdata[0] = 0x00; // Set destination network to local network
						econet::transmitFrame((econet::Frame *) &frame, rx_length);
					}
				} else {
					/* Frame is addressed to a station on another network */
//					if ((settings::relay_only_known_networks) && (econet::known_networks[frame.dst_network].network == 0)) {
						/* Don't relay the frame, but reply with &3A2 Not listening */
						econet::transmitFrame((econet::Frame *) &frame, rx_length);
//					} else {
						/* Relay frame to other known network(s) */
						aun::transmitFrame((econet::Frame *) &frame, rx_length);
//					}
				}
			}
		}
	}

	void transmitFrame(econet::Frame *frame, unsigned int size) {
		if (econet::netmon)
			netmonPrintFrame("eco  ", true, frame, size);
		api::transmitData(frame, size);
	}

	/* Check if a frame is a valid Econet frame */
	bool validateFrame(econet::Frame *frame, int size) {
		if (size < 4)
			frame->flags |= ECONET_FRAME_INVALID;
		if (size == 4)
			frame->flags |= ECONET_FRAME_ACK;
		if (size == 6)
			frame->flags |= ECONET_FRAME_SCOUT;
		if (size > 4)
			frame->flags |= ECONET_FRAME_DATA;
		if (frame->flags || ECONET_FRAME_INVALID) {
			return false;
		} else {
			frame->port = frame->rawdata[4];
			if ((frame->econet.dst_network | frame->econet.dst_station) == 0xFF)
				frame->flags |= ECONET_FRAME_BROADCAST;
			if ((frame->econet.dst_network == settings::econet_network) || (frame->econet.dst_network == 0x00)) {
				frame->flags |= ECONET_FRAME_TOLOCAL;
			if (frame->econet.dst_station == settings::econet_station) {
				frame->flags |= ECONET_FRAME_TOME;
			}
		}

			if (econet::hasSession(0, frame->econet.src_network, frame->econet.src_station, frame->port) == false) {
				/* New session */
				frame->control = frame->rawdata[4];
				frame->port = frame->rawdata[5];
				if ((frame->control & 0x80) != 0x80)
					frame->flags |= ECONET_FRAME_INVALID;
			} else {
				/* Resume session on reply port (frame has no control byte) */
				frame->control = 0x00;
			}
			return true;
		}
	}

	/* Processes one Econet frame */
	void processFrame(econet::Frame *frame, int size) {
		frame->rawdata[0] = frame->econet.src_network;
		frame->rawdata[1] = frame->econet.src_station;
		frame->rawdata[2] = settings::econet_network;
		frame->rawdata[3] = settings::econet_station;

		switch (frame->port) {
			// &00 Immediate
			case 0x00 :
				break;

			// &90 FileServerReply
			case 0x90 :
				break;

			// &91 FileServerData
			case 0x91 :
				break;

			// &93 Remote
			case 0x93 :
				break;

			// &99 FileServerCommand
			case 0x99 :
				switch (frame->control) {
					case 0x80 :	// Start a FileServerCommand session (client issued an OSWORD &14 XY+00=0 call)
						econet::startSession(0, frame->econet.src_network, frame->econet.src_station, 0x90);	// Start a session with this client
						econet::transmitFrame(frame, 4);			// Reply with ack
						break;

					default :
						break;
				}
				break;

			// &9C BRIDGE (See http://mdfs.net/Docs/Books/SJMDFS/Chapter10)
			case 0x9C :
				switch (frame->control) {
					// &80 New bridge available on the network
					case 0x80 :
						if (settings::econet_network == frame->econet.src_network) {
							fprintf(stderr, "Warning! Found another Econet network with the same network number as ours! Please check the configuration at station %i:%i\n", frame->econet.src_network, frame->econet.src_station);
						} else {
							// Add the new Econet network to our list of known networks
							econet::known_networks[frame->econet.src_network].network = frame->econet.src_network;
							econet::known_networks[frame->econet.src_network].station = frame->econet.src_station;
						}
						break;

					// &81 Reply to new bridge frame (&80)
					case 0x81 :
						break;

					// &82 What net?
					case 0x82 :
						break;

					// &83 Is net?
					case 0x83 :
						break;

					default :
						break;
				}
				break;

			// &9D ResourceLocator
			case 0x9D :
				break;

			// &9E PrinterServerEnquiryReply
			case 0x9E :
				break;

			// &9F PrinterServerEnquiry (See http://mdfs.net/Docs/Books/SJMDFS/Chapter10 / SJR_HDFSSysMgrManual.pdf $7.4)
			case 0x9F :
				break;

			// &A0 SJ *FAST protocol
			case 0xA0 :
				break;

			// &A1 SJ Nexus net find reply port
			case 0xA1 :
				break;

			// &B0 FindServer
			case 0xB0 :
				break;

			// &B1 FindServerReply
			case 0xB1 :
				break;

			// &B2 TeletextServerCommand
			case 0xB2 :
				break;

			// &B3 TeletextServerPage
			case 0xB3 :
				break;

			// &D0 PrintServerReply
			case 0xD0 :
				break;

			// &D1 PrintServerData
			case 0xD1 :
				break;

			// &D2 TCPIPOverEconet
			case 0xD2 :
				switch (frame->control) {
					// &81 IP Unicast
					case 0x81 :
						break;

					// &8E IP Broadcast Reply
					case 0x8E :
						break;

					// &8F IP Broadcast Request
					case 0x8F :
						break;

					// &A1 ARP Request
					case 0xA1 :
						break;

					// &A2 ARP Reply
					case 0xA2 :
						break;

					default :
						break;
				}
				break;

			// &D3 SIDFrameSlave
			case 0xD3 :
				break;

			// &D4 Scrollarama
			case 0xD4 :
				break;

			// &D5 Phone
			case 0xD5 :
				break;

			// &D6 BroadcastControl
			case 0xD6 :
				break;

			// &D7 BroadcastData
			case 0xD7 :
				break;

			// &D8 ImpressionLicenceChecker
			case 0xD8 :
				break;

			// &D9 DigitalServicesSquirrel
			case 0xD9 :
				break;

			// &DA SIDSecondary
			case 0xDA :
				break;

			// &DB DigitalServicesSquirrel2
			case 0xDB :
				break;

			// &DC DataDistributionControl
			case 0xDC :
				break;

			default:
				break;
		}
	}

	/* Send a broadcast frame to announce that a new bridge is available */
	void sendBridgeAnnounce(void) {
		econet::Frame frame;

		memmove((econet::Frame *) &frame, ECONET_BROADCAST_NEWBRIDGE, sizeof(ECONET_BROADCAST_NEWBRIDGE));
		frame.rawdata[0x02]	= settings::econet_network;
		frame.rawdata[0x03]	= settings::econet_station;
		frame.rawdata[0x06]	= settings::aun_network;

		econet::transmitFrame(&frame, sizeof(ECONET_BROADCAST_NEWBRIDGE));

		frame.rawdata[0x02]	= settings::aun_network;
		frame.rawdata[0x03]	= settings::aun_station;
		frame.rawdata[0x06]	= settings::econet_network;

		aun::transmitFrame(&frame, sizeof(ECONET_BROADCAST_NEWBRIDGE));
	}

	/* Send a broadcast frame to query other bridges what networks are available */
	void sendWhatNetBroadcast(void) {
		econet::Frame frame;

		memmove((econet::Frame *) &frame, ECONET_BROADCAST_WHATNET, sizeof(ECONET_BROADCAST_WHATNET));
		frame.rawdata[0x02]	= settings::econet_network;
		frame.rawdata[0x03]	= settings::econet_station;

		econet::transmitFrame(&frame, sizeof(ECONET_BROADCAST_WHATNET));

		frame.rawdata[0x02]	= settings::aun_network;
		frame.rawdata[0x03]	= settings::aun_station;

		aun::transmitFrame(&frame, sizeof(ECONET_BROADCAST_NEWBRIDGE));
	}

	/* Start a session with a client */
	bool startSession(uint32_t sequence, unsigned char network, unsigned char station, unsigned char port) {
		int i;

		i = 0;
		while (i < ECONET_MAX_SESSIONS) {
			if ((sessions[i].network == 0x00) && (sessions[i].station == 0x00) && (sessions[i].port == 0x00)) {
				sessions[i].sequence = sequence;
				sessions[i].timeout = ((unsigned long)time(NULL) + ECONET_SESSION_TIMEOUT);
				sessions[i].network = network;
				sessions[i].station = station;
				sessions[i].port = port;
				return(true);
			}
			i++;
		}
		return(false);
	}

	/* Check if a client has an open session on a port */
	bool hasSession(uint32_t sequence, unsigned char network, unsigned char station, unsigned char port) {
		int i;

		i = 0;
		while (i < ECONET_MAX_SESSIONS) {
			if ((sessions[i].network == network) && (sessions[i].station == station) && (sessions[i].port == port)) {
				if (sessions[i].timeout < (unsigned long)time(NULL)) {
					return true;
				} else {
					/* Session has timed out */
					endSession(sequence, network, station, port);
					if (ECONET_SESSION_TIMEOUT_NOTIFY) {
//						notify station at network:station that this session has timed out
					}
					return false;
				}
			}
			i++;
		}
		return false;
	}

	/* Ends a session with a client */
	bool endSession(uint32_t sequence, unsigned char network, unsigned char station, unsigned char port) {
		int i;

		i = 0;
		while (i < ECONET_MAX_SESSIONS) {
			if ((sessions[i].network == network) && (sessions[i].station == station) && (sessions[i].port == port)) {
				sessions[i].sequence = 0;
				sessions[i].timeout = 0;
				sessions[i].network = 0x00;
				sessions[i].station = 0x00;
				sessions[i].port = 0x00;
				return true;
			}
			i++;
		}
		return false;
	}

	/* &00 Immediate */
	int port00handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length) {
		char	*cli_ptr;
		int	retval;

		retval = 0;
		if ((rx_length >= 8) && (tx_length >= 12)) {
			switch (rx_data->aun.control) {
				// &01 Received text message
//				case 0x01 :
//					break;

				// &02 Show error
//				case 0x02 :
//					break;

				// &80 OSCLI
				case 0x00 :
					cli_ptr = (char *) &rx_data->aun.data[5];
					printf("port00handler OSCLI %s\n", cli_ptr);
					break;

				// &81 PEEK
				case 0x01 :
					break;

				// &82 POKE
				case 0x02 :
					break;

				// &83 JSR
				case 0x03 :
					break;

				// &84 USERPROC
				case 0x04 :
					break;

				// &85 OSPROC
				case 0x05 :
					break;

				// &86 HALT
				case 0x06 :
					break;

				// &87 CONTINUE
				case 0x07 :
					break;

				// &88 MACHINETYPE
				case 0x08 :
					tx_data->aun.data[0x00] = ECONET_MACHINETYPE >> 8;			// Machine type MSB
					tx_data->aun.data[0x01] = ECONET_MACHINETYPE && 0xff;			// Machine type LSB
					tx_data->aun.data[0x02] = strtol(FILESTORE_VERSION_MINOR, NULL, 10);	// Version
					tx_data->aun.data[0x03] = strtol(FILESTORE_VERSION_MAJOR, NULL, 10);	// Version
					retval = 4;
					break;

				// &89 GETREGISTERS
				case 0x09 :
					break;

				default :
					break;
			}
		}

		return retval;
	}

	// &90 FileServerReply
	int port90handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length) {
		int retval;

		retval = 0;
		if ((rx_length >= 8) && (tx_length >= 10)) {
			tx_data->aun.data[0x00] = 0x00;
			tx_data->aun.data[0x01] = 0x00;
			retval = 2;
		}

		return retval;
	}

	// &91 FileServerData
	int port91handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length) {
		int retval;
		FILE *fp_fileserverdata;

		retval = 0;
		if ((rx_length >= 8) && (tx_length >= 13)) {
			if (rx_data->aun.data[0x05] != 0) {
				if ((fp_fileserverdata = fopen(FILESERVERTMPFILE, "wb")) != NULL) {
					fwrite(rx_data->aun.data, sizeof(uint8_t), rx_length - 8, fp_fileserverdata);
					fclose(fp_fileserverdata);
					tx_data->aun.port = port99save_replyport;	// Set reply port number
					tx_data->aun.data[0x00] = 0x00;
					tx_data->aun.data[0x01] = 0x00;
					tx_data->aun.data[0x02] = 0x00;
					tx_data->aun.data[0x03] = 0x00;
					tx_data->aun.data[0x04] = 0x00;
					retval = 5;
					port99save_replyport = 0;
				} else {
					fprintf(stderr, "port91handler: can't open temp file for writing FileServerData\n");
					retval = returnError(tx_data->aun.data, tx_length, 0x000000C7);
				}
			} else {
				retval = returnError(tx_data->aun.data, tx_length, 0x000000CF);
			}
		}
		return retval;
	}

	/* &99 FileServerCommand */
	int port99handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length) {
		uint32_t loadaddr, execaddr, length, offset, size;
		char filename[ECONET_MAX_FILENAME_LEN+1];
		char disctitle[ECONET_MAX_DISCTITLE_LEN+1];
		char username[MAX_USERNAME+1];
		char access_string[9];
		uint8_t num_drives;
		uint8_t access_byte = 0x02;
		uint16_t max_block_size = 0xFFFF;
		struct tm *timeinfo;
		char **args = NULL;
		char *cli_ptr;
		int retval, result;
		size_t i;

		retval = 0;
		// Check the OSWORD &14 function code at XY+00
		switch (rx_data->aun.function) {
			// &00: Command line decoding (SJ Research page 10-49)
			case 0x00 :
				if (rx_length > 13) {
					/* Split command line into tokens */
					cli_ptr = (char *)&rx_data->aun.data[5];
					args = tokenizeCommandLine(cli_ptr);
//					rx_data->aun.data[5 + strcspn((const char *)&rx_data->aun.data[5], "\r")] = 0x00;
int	argv = 0;
while (args[argv] != NULL) {
	printf("arg%i = %s\n", argv, args[argv]);
	argv++;
}
					/* Execute command */
					executeCommand(args);

					free(args);

					tx_data->aun.data[0x00] = 0;			// Return command
					tx_data->aun.data[0x01] = 0 & 0x000000FF;	// Result
					retval = 2;
				}
				break;

			// &01: Save (SJ Research page 10-48)
			case 0x01 :
			// &1D: Create file of specified size
			case 0x1D :
				if (rx_length > 24) {
					loadaddr = rx_data->aun.data[0x05] | (rx_data->aun.data[0x06] << 8) | (rx_data->aun.data[0x07] << 16) | (rx_data->aun.data[0x08] << 24);
					execaddr = rx_data->aun.data[0x09] | (rx_data->aun.data[0x0A] << 8) | (rx_data->aun.data[0x0B] << 16) | (rx_data->aun.data[0x0C] << 24);
					length   = rx_data->aun.data[0x0D] | (rx_data->aun.data[0x0E] << 8) | (rx_data->aun.data[0x0F] << 16);
					strlcpy(filename, (const char *) &rx_data->aun.data[0x10], rx_length - 0x18);

					fprintf(stderr, "&99-&01 *SAVE %s %08X %08X %06X\n%lu\n", filename, loadaddr, execaddr, length, rx_length - 0x18);
//					startSession(0, 0, 0, rx_data->aun.data[0x00]);
					port99save_replyport = rx_data->aun.data[0x00];

//					if (netfs::save(filename) == 0) {
						tx_data->aun.data[0x00] = 0;						// Return command
						tx_data->aun.data[0x01] = 0;						// Result
						tx_data->aun.data[0x02] = 0x91;						// Data port (=&9F on FileServer level 1)
						if (length > max_block_size) {						// TODO: find out how file transfers bigger than max_block_size (multi-packet file transfers) are handled
							tx_data->aun.data[0x03] = (max_block_size & 0x00FF);		// Max size of data block per packet LSB
							tx_data->aun.data[0x04] = (max_block_size & 0xFF00) >> 8;	// Max size of data block per packet MSB
						} else {
							tx_data->aun.data[0x03] = (length & 0x00FF);			// Max size of data block per packet LSB
							tx_data->aun.data[0x04] = (length & 0xFF00) >> 8;		// Max size of data block per packet MSB
						}
						strlcpy((char *)&tx_data->aun.data[0x05], filename, tx_length - 7);
						tx_data->aun.data[0x05 + strlen(filename)] = 0x0D;
						retval = 6 + strlen(filename);
//					} else {
//						retval = returnError(tx_data->aun.data, tx_length, 0x000000BA);
//					}
				}
				break;

			// &02: Load
			case 0x02 :
			// &05: Load as command
			case 0x05 :
				if (rx_length > 13) {
					strlcpy(filename, (const char *) &rx_data->aun.data[0x05], rx_length - 0x0D);

//					if (netfs::save(filename) == 0) {
						loadaddr = 0xFFFF1900;
						execaddr = 0xFFFF8023;
						length   = 0x00004C85;

						tx_data->aun.data[0x00] = 0x00;							// Command
						tx_data->aun.data[0x01] = 0x00;							// Error code
						tx_data->aun.data[0x02] = (loadaddr & 0x000000FF);				// Load address LSB
						tx_data->aun.data[0x03] = (loadaddr & 0x0000FF00) >> 8;				// Load address
						tx_data->aun.data[0x04] = (loadaddr & 0x00FF0000) >> 16;			// Load address
						tx_data->aun.data[0x05] = (loadaddr & 0xFF000000) >> 24;			// Load address MSB
						tx_data->aun.data[0x06] = (execaddr & 0x000000FF);				// Exec address LSB
						tx_data->aun.data[0x07] = (execaddr & 0x0000FF00) >> 8;				// Exec address
						tx_data->aun.data[0x08] = (execaddr & 0x00FF0000) >> 16;			// Exec address
						tx_data->aun.data[0x09] = (execaddr & 0xFF000000) >> 24;			// Exec address MSB
						tx_data->aun.data[0x0A] = (length   & 0x000000FF);				// Length LSB
						tx_data->aun.data[0x0B] = (length   & 0x0000FF00) >> 8;				// Length
						tx_data->aun.data[0x0C] = (length   & 0x00FF0000) >> 16;			// Length MSB
						tx_data->aun.data[0x0D] = access_byte;						// Access byte LWRwr (bottom 5 bits)
						tx_data->aun.data[0x0E] = 0;							// File creation date: day
						tx_data->aun.data[0x0F] = 0;							// File creation date: year (4 bits), month (4 bits)
						strlcpy((char *)&tx_data->aun.data[0x10], filename, ECONET_MAX_FILENAME_LEN);	// Object name, padded with spaces
//					} else {
//						retval = returnError(tx_data->aun.data, tx_length, 0x000000D6);
//					}
				}
				break;

			// &03: Examine
			case 0x03 :
				if (rx_length > 16) {
					uint8_t entrypoint  = rx_data->aun.data[0x06];
					uint8_t numentries  = rx_data->aun.data[0x07];
					strlcpy(filename, (const char *) &rx_data->aun.data[0x08], rx_length - 0x10);
fprintf(stderr, "entry=%i numentries=%i dirname=%s\n", entrypoint, numentries, filename);

					loadaddr = 0xFFFF1900;
					execaddr = 0xFFFF8023;
					length   = 0x00004C85;
					strcpy(filename, "abcdefghij");
					strcpy(access_string, "MPDLWRwr");

					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code
					tx_data->aun.data[0x02] = 0x01;					// Number of objects returned

					switch (rx_data->aun.data[5]) {
						// All information, machine readable format
						case 0x00 :
							strlcpy((char *)&tx_data->aun.data[0x03], filename, ECONET_MAX_FILENAME_LEN);	// Object name, padded with spaces
							tx_data->aun.data[0x0D] = (loadaddr & 0x000000FF);				// Load address LSB
							tx_data->aun.data[0x0E] = (loadaddr & 0x0000FF00) >> 8;				// Load address
							tx_data->aun.data[0x0F] = (loadaddr & 0x00FF0000) >> 16;			// Load address
							tx_data->aun.data[0x10] = (loadaddr & 0xFF000000) >> 24;			// Load address MSB
							tx_data->aun.data[0x11] = (execaddr & 0x000000FF);				// Exec address LSB
							tx_data->aun.data[0x12] = (execaddr & 0x0000FF00) >> 8;				// Exec address
							tx_data->aun.data[0x13] = (execaddr & 0x00FF0000) >> 16;			// Exec address
							tx_data->aun.data[0x14] = (execaddr & 0xFF000000) >> 24;			// Exec address MSB
							tx_data->aun.data[0x15] = access_byte;						// Access byte LWRwr (bottom 5 bits)
							tx_data->aun.data[0x16] = 0;							// Date: day
							tx_data->aun.data[0x17] = 0;							// Date: year (4 bits), month (4 bits)
							tx_data->aun.data[0x18] = 0;							// System internal name
							tx_data->aun.data[0x19] = 0;							// System internal name
							tx_data->aun.data[0x1A] = 0;							// System internal name
							tx_data->aun.data[0x1B] = (length   & 0x000000FF);				// Length LSB
							tx_data->aun.data[0x1C] = (length   & 0x0000FF00) >> 8;				// Length
							tx_data->aun.data[0x1D] = (length   & 0x00FF0000) >> 16;			// Length MSB
							retval = 29;
							break;

						// All information, character string
						case 0x01 :
							sprintf((char *) &tx_data->aun.data[0x03], "%s %08X %08X %06X %s\x80", filename, loadaddr, execaddr, length, access_string);
							retval = (strlen((char *) &tx_data->aun.data[0x03]) - 1);	// Skip training \0
							break;

						// File title only
						case 0x02 :
							tx_data->aun.data[0x03] = 0x0A;							// Length of filename
							strlcpy((char *) &tx_data->aun.data[0x04], filename, ECONET_MAX_FILENAME_LEN);	// Object name, padded with spaces
							retval = 13;
							break;

						// File title and access, character string
						case 0x03 :
							tx_data->aun.data[0x03] = 0x0A;							// ???????
							strlcpy((char *) &tx_data->aun.data[0x04], filename, ECONET_MAX_FILENAME_LEN+1);	// Object name, padded with spaces
							strlcpy((char *) &tx_data->aun.data[0x0E], access_string, 9);				// Access string
							tx_data->aun.data[0x16] = 0x80;							// Error code (0 = success)
							retval = 23;
							break;

						default:
							retval = returnError(tx_data->aun.data, tx_length, 0x000000CF);			// Error code &CF: Bad attribute
							break;
					}
				}
				break;

			// &04: Read catalogue header
			case 0x04 :
				if (rx_length > 13) {
					strlcpy(disctitle, "EliteDisc       ", ECONET_MAX_DISCTITLE_LEN + 1);		// Name of currently selected disc, padded with spaces

					strlcpy(filename, (const char *) &rx_data->aun.data[0x05], rx_length - 0x0D);

					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code
					strlcpy((char *)&tx_data->aun.data[0x02], "LastObject", ECONET_MAX_FILENAME_LEN);	// Object name, padded with spaces
					tx_data->aun.data[0x0C] = 0xFF;					// Access: &00=Owner access, &FF=Public access
					tx_data->aun.data[0x0D] = 0x20;					// Padding
					tx_data->aun.data[0x0E] = 0x20;					// Padding
					tx_data->aun.data[0x0F] = 0x20;					// Padding
					strlcpy((char *)&tx_data->aun.data[0x10], disctitle, ECONET_MAX_DISCTITLE_LEN);
				}
				break;

			// &06: Open file
			case 0x06 :
				if (rx_length > 15) {
					strlcpy(filename, (const char *) &rx_data->aun.data[0x07], rx_length - 0x0F);

					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code
					tx_data->aun.data[0x02] = 0x01;					// File handle

					retval = 3;
				}
				break;

			// &07: Close file
			case 0x07 :
				if (rx_length == 14) {
					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code

					retval = 2;
				}
				break;

			// &08: Get byte
			case 0x08 :
				if (rx_length == 14) {
					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code
					tx_data->aun.data[0x02] = 0x01;					// Byte read (&FE if first byte after EOF)
					tx_data->aun.data[0x03] = 0x01;					// Flag read (&00=normal byte, &80=last byte in file, &C0=first byte after EOF)

					retval = 4;
				}
				break;

			// &09: Put byte
			case 0x09 :
				if (rx_length == 15) {
					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code

					retval = 2;
				}
				break;

			// &0A: Get multiple bytes
			case 0x0A :
				if (rx_length == 21) {
					length = rx_data->aun.data[0x06] | (rx_data->aun.data[0x07] << 8) | (rx_data->aun.data[0x08] << 16);
					if (rx_data->aun.data[0x05] == 0x00)
						offset = rx_data->aun.data[0x09] | (rx_data->aun.data[0x0A] << 8) | (rx_data->aun.data[0x0B] << 16);

					tx_data->aun.data[0x00] = 0;						// Return command
					tx_data->aun.data[0x01] = 0;						// Result
					tx_data->aun.data[0x02] = 0x90;						// Data port
					tx_data->aun.data[0x03] = (max_block_size & 0x00FF);			// Max size of data block per packet LSB
					tx_data->aun.data[0x04] = (max_block_size & 0xFF00) << 8;		// Max size of data block per packet MSB

					retval = 5;
				}
				break;

			// &0B: Put multiple bytes
			case 0x0B :
				if (rx_length == 21) {
					length = rx_data->aun.data[0x06] | (rx_data->aun.data[0x07] << 8) | (rx_data->aun.data[0x08] << 16);
					if (rx_data->aun.data[0x05] == 0x00)
						offset = rx_data->aun.data[0x09] | (rx_data->aun.data[0x0A] << 8) | (rx_data->aun.data[0x0B] << 16);

					tx_data->aun.data[0x00] = 0;						// Return command
					tx_data->aun.data[0x01] = 0;						// Result
					tx_data->aun.data[0x02] = 0x90;						// Data port
					tx_data->aun.data[0x03] = (max_block_size & 0x00FF);			// Max size of data block per packet LSB
					tx_data->aun.data[0x04] = (max_block_size & 0xFF00) << 8;		// Max size of data block per packet MSB

					retval = 5;
				}
				break;

			// &0C: Read random access information
			case 0x0C :
				if (rx_length == 15) {
					tx_data->aun.data[0x00] = 0x00;							// Command
					tx_data->aun.data[0x01] = 0x00;							// Error code

					length = 0x00004000;

					switch (rx_data->aun.data[0x06]) {
						/* TODO: Return sequential file pointer (PTR#) */
						case 0x00 :
							tx_data->aun.data[0x02] = (length   & 0x000000FF);		// Length LSB
							tx_data->aun.data[0x03] = (length   & 0x0000FF00) >> 8;		// Length
							tx_data->aun.data[0x04] = (length   & 0x00FF0000) >> 16;	// Length MSB
							break;

						/* TODO: Return file extent (amount of valid data) */
						case 0x01 :
							tx_data->aun.data[0x02] = (length   & 0x000000FF);		// Length LSB
							tx_data->aun.data[0x03] = (length   & 0x0000FF00) >> 8;		// Length
							tx_data->aun.data[0x04] = (length   & 0x00FF0000) >> 16;	// Length MSB
							break;

						/* Return file size (the space allocated for the file) */
						case 0x02 :
							tx_data->aun.data[0x02] = (length   & 0x000000FF);		// Length LSB
							tx_data->aun.data[0x03] = (length   & 0x0000FF00) >> 8;		// Length
							tx_data->aun.data[0x04] = (length   & 0x00FF0000) >> 16;	// Length MSB
							break;

						default :
							retval = returnError(tx_data->aun.data, tx_length, 0x000000CF);
							break;
					}
					retval = 5;
				}
				break;

			// &0D: Set random access information
			case 0x0D :
				if (rx_length == 18) {
					tx_data->aun.data[0x00] = 0x00;							// Command
					tx_data->aun.data[0x01] = 0x00;							// Error code
					length = tx_data->aun.data[0x02] | (tx_data->aun.data[0x03] << 8) | (tx_data->aun.data[0x04] << 16);

					switch (rx_data->aun.data[0x06]) {
						/* TODO: Set sequential file pointer (PTR#) */
						case 0x00 :
							break;

						/* TODO: Set file extent (amount of valid data) */
						case 0x01 :
							break;

						default :
							retval = returnError(tx_data->aun.data, tx_length, 0x000000CF);
							break;
					}
					retval = 5;
				}
				break;

			// &0E: Read disc name information
			case 0x0E :
				if (rx_length == 15) {
					if (rx_data->aun.data[0x05] < ECONET_MAX_DISCDRIVES) {
						strlcpy(disctitle, "EliteDisc       ", ECONET_MAX_DISCTITLE_LEN + 1);		// Name of currently selected disc, padded with spaces

						tx_data->aun.data[0x00] = 0x00;	// Command
						tx_data->aun.data[0x01] = 0x00;	// Error code (0 = success)
						tx_data->aun.data[0x02] = 0x01;	// Number of drives found
						tx_data->aun.data[0x03] = 0x30;	// Number of drives found
						strlcpy((char *)&tx_data->aun.data[0x04], disctitle, ECONET_MAX_DISCTITLE_LEN);
//						num_drives = rx_data->aun.data[6];
//						for (i = rx_data->aun.data[5]; i < ECONET_MAX_DISCDRIVES; i++) {
//							tx_data->aun.data[6] = i;
//							strncpy((char *)&tx_data->aun.data[7 + ((i - first_drive) * ECONET_MAX_DISCTITLE_LEN)], netfs::getDiscTitle(i), ECONET_MAX_DISCTITLE_LEN);
//						}
						retval = 20;
					} else {
						tx_data->aun.data[0x00] = 0x00;	// Command
						tx_data->aun.data[0x01] = 0x00;	// Error code (0 = success)
						tx_data->aun.data[0x02] = 0x00;	// No drives found

						retval = 3;
					}
				}
				break;

			// &0F: Read logged on users
			case 0x0F :
				if (rx_length == 15) {
					tx_data->aun.data[0] = 0x00;	// Command
					tx_data->aun.data[1] = 0x00;	// Error code (0 = success)
					tx_data->aun.data[2] = users::totalSessions;	// Number of user sessions
					retval = 0x03;
					for (i = 0; i < users::totalSessions; i++) {
						tx_data->aun.data[retval++] = users::sessions[i].network;
						tx_data->aun.data[retval++] = users::sessions[i].network;
						sprintf((char *)&tx_data->aun.data[retval], "%s\r", users::users[users::sessions[i].user_id].username);
						retval += strlen(users::users[users::sessions[i].user_id].username);
						users::users[users::sessions[i].user_id].flags.p ? tx_data->aun.data[retval++] = 0x00 : tx_data->aun.data[retval++] = 0xFF;
					}
				}
				break;

			// &10: Read date/time
			case 0x10 :
				if (rx_length == 13) {
					time_t rawtime;
					time(&rawtime);
					timeinfo = localtime(&rawtime);

					tx_data->aun.data[0x00] = 0x00;							// Command
					tx_data->aun.data[0x01] = 0x00;							// Error code
					tx_data->aun.data[0x02] = timeinfo->tm_mday;
					tx_data->aun.data[0x03] = ((timeinfo->tm_year & 0x0F) << 4) | timeinfo->tm_mon;
					tx_data->aun.data[0x04] = timeinfo->tm_hour;
					tx_data->aun.data[0x05] = timeinfo->tm_min;
					tx_data->aun.data[0x06] = timeinfo->tm_sec;
					retval = 7;
				}
				break;

			// &11: TODO: Read EOF (End Of File) information
			case 0x11 :
				if (rx_length == 14) {
					tx_data->aun.data[0x00] = 0x00;							// Command
					tx_data->aun.data[0x01] = 0x00;							// Error code
					tx_data->aun.data[0x02] = 0x00;							// EOF flag (&00=file pointer within file, &FF=file pointer outside file)

					retval = 3;
				}
				break;

			// &12: Read object information
			case 0x12 :
				if (rx_length > 14) {
					strlcpy(filename, (const char *) &rx_data->aun.data[0x06], rx_length - 0x0E);
					loadaddr = 0xFFFF1900;
					execaddr = 0xFFFF8023;
					length   = 0x00004C85;

					switch (rx_data->aun.data[0x05]) {
						// &01: Read object creation date
						case 0x01 :
							tx_data->aun.data[0x00] = 0;					// Return command
							tx_data->aun.data[0x01] = 0;					// Result
							tx_data->aun.data[0x02] = 1;					// 0=Object not found, 1=Object is a file, 2=Object is a directory
							tx_data->aun.data[0x03] = 0;					// Date: day
							tx_data->aun.data[0x04] = 0;					// Date: year (4 bits), month (4 bits)

							retval = 0x05;
							break;

						// &02: Read load and execute address
						case 0x02 :
							tx_data->aun.data[0x00] = 0;					// Return command
							tx_data->aun.data[0x01] = 0;					// Result
							tx_data->aun.data[0x02] = 1;					// 0=Object not found, 1=Object is a file, 2=Object is a directory
							tx_data->aun.data[0x03] = (loadaddr & 0x000000FF);		// Load address LSB
							tx_data->aun.data[0x04] = (loadaddr & 0x0000FF00) >> 8;		// Load address
							tx_data->aun.data[0x05] = (loadaddr & 0x00FF0000) >> 16;	// Load address
							tx_data->aun.data[0x06] = (loadaddr & 0xFF000000) >> 24;	// Load address MSB
							tx_data->aun.data[0x07] = (execaddr & 0x000000FF);		// Exec address LSB
							tx_data->aun.data[0x08] = (execaddr & 0x0000FF00) >> 8;		// Exec address
							tx_data->aun.data[0x09] = (execaddr & 0x00FF0000) >> 16;	// Exec address
							tx_data->aun.data[0x0A] = (execaddr & 0xFF000000) >> 24;	// Exec address MSB

							retval = 0x0B;
							break;

						// &03: Read object extent (three bytes only)
						case 0x03 :
							tx_data->aun.data[0x00] = 0;					// Return command
							tx_data->aun.data[0x01] = 0;					// Result
							tx_data->aun.data[0x02] = 2;					// 0=Object not found, 1=Object is a file, 2=Object is a directory
							tx_data->aun.data[0x03] = 0;					// Object extent
							tx_data->aun.data[0x04] = 0;					// Object extent
							tx_data->aun.data[0x05] = 0;					// Object extent

							retval = 0x06;
							break;

						// &04: Read access byte (as for EXAMINE)
						case 0x04 :
							tx_data->aun.data[0x00] = 0;					// Return command
							tx_data->aun.data[0x01] = 0;					// Result
							tx_data->aun.data[0x02] = 2;					// 0=Object not found, 1=Object is a file, 2=Object is a directory
							tx_data->aun.data[0x03] = access_byte;				// Access byte

							retval = 0x04;
							break;

						// &05: Read all object attributes
						case 0x05 :
							loadaddr = 0xFFFF1900;
							execaddr = 0xFFFF8023;
							tx_data->aun.data[0x00] = 0;					// Return command
							tx_data->aun.data[0x01] = 0;					// Result
							if ((strcmp(filename, "$") == 0) || (strlen(filename) == 0))
								tx_data->aun.data[0x02] = 2;				// 0=Object not found, 1=Object is a file, 2=Object is a directory
							else
								tx_data->aun.data[0x02] = 1;				// 0=Object not found, 1=Object is a file, 2=Object is a directory
							tx_data->aun.data[0x03] = (loadaddr & 0x000000FF);		// Load address LSB
							tx_data->aun.data[0x04] = (loadaddr & 0x0000FF00) >> 8;		// Load address
							tx_data->aun.data[0x05] = (loadaddr & 0x00FF0000) >> 16;	// Load address
							tx_data->aun.data[0x06] = (loadaddr & 0xFF000000) >> 24;	// Load address MSB
							tx_data->aun.data[0x07] = (execaddr & 0x000000FF);		// Exec address LSB
							tx_data->aun.data[0x08] = (execaddr & 0x0000FF00) >> 8;		// Exec address
							tx_data->aun.data[0x09] = (execaddr & 0x00FF0000) >> 16;	// Exec address
							tx_data->aun.data[0x0A] = (execaddr & 0xFF000000) >> 24;	// Exec address MSB
							tx_data->aun.data[0x0B] = (length   & 0x000000FF);		// Length LSB
							tx_data->aun.data[0x0C] = (length   & 0x0000FF00) >> 8;		// Length
							tx_data->aun.data[0x0D] = (length   & 0x00FF0000) >> 16;	// Length MSB
							tx_data->aun.data[0x0E] = access_byte;				// Access byte
							tx_data->aun.data[0x0F] = 0;					// Date: day
							tx_data->aun.data[0x10] = 0;					// Date: year (4 bits), month (4 bits)
							tx_data->aun.data[0x11] = 0xFF;					// Access: &00=Owner access, &FF=Public access

							retval = 0x12;
							break;

						// &06: Read access and cycle number of directory
						case 0x06 :
							tx_data->aun.data[0x00] = 0;					// Return command; undefined
							tx_data->aun.data[0x01] = 0;					// Result
							strlcpy((char *)&tx_data->aun.data[0x02], "$         ", 11);	// Directory name, padded with spaces
							tx_data->aun.data[0x0C] = 0xFF;					// Access: &00=Owner access, &FF=Public access
							tx_data->aun.data[0x0D] = 0x3A;					// Cycle number of directory

							retval = 0x0E;
							break;

						// &40: Read creation and update time
						case 0x40 :
							tx_data->aun.data[0x00] = 0;					// Return command
							tx_data->aun.data[0x01] = 0;					// Result
							tx_data->aun.data[0x02] = 1;					// 0=Object not found, 1=Object is a file, 2=Object is a directory
							tx_data->aun.data[0x03] = 0;					// Creation date: day
							tx_data->aun.data[0x04] = 0;					// Creation date: year (4 bits), month (4 bits)
							tx_data->aun.data[0x05] = 0;					// Creation date: hours
							tx_data->aun.data[0x06] = 0;					// Creation date: minutes
							tx_data->aun.data[0x07] = 0;					// Creation date: seconds
							tx_data->aun.data[0x08] = 0;					// Modify date: day
							tx_data->aun.data[0x09] = 0;					// Modify date: year (4 bits), month (4 bits)
							tx_data->aun.data[0x0A] = 0;					// Modify date: hours
							tx_data->aun.data[0x0B] = 0;					// Modify date: minutes
							tx_data->aun.data[0x0C] = 0;					// Modify date: seconds

							retval = 0x0D;
							break;

						default :
							retval = returnError(tx_data->aun.data, tx_length, 0x000000CF);
							break;
					}
				}
				break;

			// &13: Set object information
			case 0x13 :
				if (rx_length > 14) {
					switch (rx_data->aun.data[0x05]) {
						// &01: Set load/exec/access
						case 0x01 :
							loadaddr = tx_data->aun.data[0x06] | (tx_data->aun.data[0x07] << 8) | (tx_data->aun.data[0x08] << 16)| (tx_data->aun.data[0x09] << 24);
							execaddr = tx_data->aun.data[0x0A] | (tx_data->aun.data[0x0B] << 8) | (tx_data->aun.data[0x0C] << 16)| (tx_data->aun.data[0x0D] << 24);
							access_byte = tx_data->aun.data[0x0E];
							strlcpy(filename, (const char *) &rx_data->aun.data[0x0F], rx_length - 0x17);
							break;

						// &02: Set load address
						case 0x02 :
							loadaddr = tx_data->aun.data[0x06] | (tx_data->aun.data[0x07] << 8) | (tx_data->aun.data[0x08] << 16)| (tx_data->aun.data[0x09] << 24);
							strlcpy(filename, (const char *) &rx_data->aun.data[0x0A], rx_length - 0x12);
							break;

						// &03: Set exec address
						case 0x03 :
							execaddr = tx_data->aun.data[0x06] | (tx_data->aun.data[0x07] << 8) | (tx_data->aun.data[0x08] << 16)| (tx_data->aun.data[0x09] << 24);
							strlcpy(filename, (const char *) &rx_data->aun.data[0x0A], rx_length - 0x12);
							break;

						// &04: Set access
						case 0x04 :
							access_byte = tx_data->aun.data[0x06];
							strlcpy(filename, (const char *) &rx_data->aun.data[0x07], rx_length - 0x0F);
							break;

						// &05: Set creation date
						case 0x05 :
							strlcpy(filename, (const char *) &rx_data->aun.data[0x08], rx_length - 0x10);
							break;

						// &40: Set modify/creation date and time
						case 0x40 :
							strlcpy(filename, (const char *) &rx_data->aun.data[0x10], rx_length - 0x18);
							break;

						default :
							retval = returnError(tx_data->aun.data, tx_length, 0x000000CF);
							break;
					}
					tx_data->aun.data[0x00] = 0;					// Return command; undefined
					tx_data->aun.data[0x01] = 0;					// Result

					retval = 2;
				}
				break;

			// &14: Delete object
			case 0x14 :
				if (rx_length > 13) {
					strlcpy(filename, (const char *) &rx_data->aun.data[0x05], rx_length - 0x0D);

//					if (netfs::del(filename) == 0) {
						loadaddr = 0xFFFF1900;
						execaddr = 0xFFFF8023;
						length   = 0x00004C85;
						tx_data->aun.data[0x00] = 0x00;					// Command
						tx_data->aun.data[0x01] = 0x00;					// Error code
						tx_data->aun.data[0x02] = (loadaddr & 0x000000FF);		// Load address LSB
						tx_data->aun.data[0x03] = (loadaddr & 0x0000FF00) >> 8;		// Load address
						tx_data->aun.data[0x04] = (loadaddr & 0x00FF0000) >> 16;	// Load address
						tx_data->aun.data[0x05] = (loadaddr & 0xFF000000) >> 24;	// Load address MSB
						tx_data->aun.data[0x06] = (execaddr & 0x000000FF);		// Exec address LSB
						tx_data->aun.data[0x07] = (execaddr & 0x0000FF00) >> 8;		// Exec address
						tx_data->aun.data[0x08] = (execaddr & 0x00FF0000) >> 16;	// Exec address
						tx_data->aun.data[0x09] = (execaddr & 0xFF000000) >> 24;	// Exec address MSB
						tx_data->aun.data[0x0A] = (length   & 0x000000FF);		// Length LSB
						tx_data->aun.data[0x0B] = (length   & 0x0000FF00) >> 8;		// Length
						tx_data->aun.data[0x0C] = (length   & 0x00FF0000) >> 16;	// Length MSB
						retval = 13;
//					} else {
//						retval = returnError(tx_data->aun.data, tx_length, 0x000000D6);
//					}
				}
				break;

			// &15: Read user environment
			case 0x15 :
				// Rx length should be only 13, but a bug in NFS3.60 sends a packet with extra junk (61 bytes in total)
				if (rx_length >= 13) {
					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code
					tx_data->aun.data[0x02] = ECONET_MAX_DISCTITLE_LEN;		// Length of disc name
					strlcpy((char *)&tx_data->aun.data[0x03], "EliteDisc       ", ECONET_MAX_DISCTITLE_LEN + 1);	// Name of currently selected disc, padded with spaces
					strlcpy((char *)&tx_data->aun.data[0x03 + ECONET_MAX_DISCTITLE_LEN     ], "$         ", 11);	// Name of CSD, padded with spaces
					strlcpy((char *)&tx_data->aun.data[0x03 + ECONET_MAX_DISCTITLE_LEN + 10], "&         ", 11);	// Name of LIB, padded with spaces
					retval = 39;
				}
				break;

			// &16: Set user's boot option
			case 0x16 :
				if (rx_length == 14) {
					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code

					retval = 2;
				}
				break;

			// &17: Log off
			case 0x17 :
				if (rx_length == 13) {
					tx_data->aun.data[0] = 0x00;							// Command
//					if ((users::delSession(users::getSession(0, 0, 0))) == 0) {
						tx_data->aun.data[1] = 0x00;						// Error code
						retval = 2;
//					} else {
//						retval = returnError(tx_data->aun.data, tx_length, 0x000000AE);
//					}
				}
				break;

			// &18: Read user information
			case 0x18 :
				if (rx_length > 13) {
					strlcpy(username, (const char *) &rx_data->aun.data[0x05], rx_length - 0x0D);
					if (username[strlen(username)] == '\r')						// Strip trailing CR
						username[strlen(username)] = '\0';

					if ((result = users::getUserID(username)) >= 0) {
						tx_data->aun.data[0x00] = 0x00;						// Command
						tx_data->aun.data[0x01] = 0x00;						// Error code
						users::users[result].flags.p ? tx_data->aun.data[0x02] = 0x00 : tx_data->aun.data[0x02] = 0xFF;
						tx_data->aun.data[0x03] = 0xFF;						// Set to &FF if user is not logged in
						tx_data->aun.data[0x04] = 0xFF;						// Set to &FF if user is not logged in
						for (i = 0; i < users::totalSessions; i++) {				// Find first station that the user is logged into TODO: how to handle users which are logged into multiple stations?
							if (users::sessions[i].user_id == (uint32_t)result) {
								tx_data->aun.data[0x03] = users::sessions[i].network;
								tx_data->aun.data[0x04] = users::sessions[i].station;
							}
						}
						retval = 5;
					} else {
						retval = returnError(tx_data->aun.data, tx_length, 0x000000BC);
					}
				}
				break;

			// &19: Read file server version number
			case 0x19 :
				if (rx_length == 13) {
					sprintf((char *)&tx_data->aun.data[5], "v" FILESTORE_VERSION_MAJOR "." FILESTORE_VERSION_MINOR "." FILESTORE_VERSION_PATCHLEVEL);
					retval = strlen((const char *) &tx_data->aun.data[5]);
				}
				break;

			// &1A: Read file server free space
			case 0x1A :
				if (rx_length > 13) {
					strlcpy(disctitle, (const char *) &rx_data->aun.data[0x05], ECONET_MAX_DISCTITLE_LEN + 1);		// Name of currently selected disc, padded with spaces
					if (username[strlen(disctitle)] == '\r')				// Strip trailing CR
						username[strlen(disctitle)] = '\0';

					length = 0x00000500;

					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code
					tx_data->aun.data[0x02] = (length   & 0x000000FF);		// Free space on disc LSB
					tx_data->aun.data[0x03] = (length   & 0x0000FF00) >> 8;		// Free space on disc
					tx_data->aun.data[0x04] = (length   & 0x00FF0000) >> 16;	// Free space on disc MSB
					tx_data->aun.data[0x05] = (length   & 0x000000FF);		// Total size of disc LSB
					tx_data->aun.data[0x06] = (length   & 0x0000FF00) >> 8;		// Total size of disc
					tx_data->aun.data[0x07] = (length   & 0x00FF0000) >> 16;	// Total size of disc MSB

					retval = 8;
				}
				break;

			// &1B: Create directory
			case 0x1B :
				if (rx_length > 14) {
					strlcpy(filename, (const char *) &rx_data->aun.data[0x06], rx_length - 0x0E);

					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code

					retval = 2;
				}
				break;

			// &1C: Set date/time
			case 0x1C :
				if (rx_length > 18) {
					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code

					retval = 2;
				}
				break;

			// &1E: Read user free space
			case 0x1E :
				if (rx_length > 13) {
					strlcpy(username, (const char *) &rx_data->aun.data[0x05], rx_length - 0x0D);

					size = 0x00010000;

					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code
					tx_data->aun.data[0x02] = (size & 0x000000FF);		// Available space for specified user LSB
					tx_data->aun.data[0x03] = (size & 0x0000FF00) >> 8;		// Available space for specified user
					tx_data->aun.data[0x04] = (size & 0x00FF0000) >> 16;	// Available space for specified user
					tx_data->aun.data[0x05] = (size & 0xFF000000) >> 24;	// Available space for specified user MSB

					retval = 6;
				}
				break;

			// &1F: Set user free space
			case 0x1F :
				if (rx_length > 17) {
					size = rx_data->aun.data[0x05] | (rx_data->aun.data[0x06] << 8) | (rx_data->aun.data[0x07] << 16) | (rx_data->aun.data[0x08] << 24);
					strlcpy(username, (const char *) &rx_data->aun.data[0x09], rx_length - 0x11);

					tx_data->aun.data[0x00] = 0x00;					// Command
					tx_data->aun.data[0x01] = 0x00;					// Error code
					retval = 2;
				}
				break;

			// &20: Read client user identifier
			case 0x20 :
				if (rx_length == 13) {
					for (i = 0; i < users::totalSessions; i++) {
						if ((users::sessions[i].network == rx_data->econet.src_network) && (users::sessions[i].network == rx_data->econet.src_station)) {
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code
							strlcpy((char *)&tx_data->aun.data[0x02], users::users[users::sessions[i].user_id].username, tx_length - 0x03);
							retval = 0x02 + strlen(users::users[users::sessions[i].user_id].username);
							tx_data->aun.data[retval] = '\r';
							retval++;
						}
					}
					if (retval == 0) {
						retval = returnError(tx_data->aun.data, tx_length, 0x000000AE);
					}
				}
				break;

			// &40: Read account information
			case 0x40 :
				if (rx_length == 19) {
					switch (rx_data->aun.data[0x05]) {
						// &00: Read account info
						case 0x00 :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						default :
							retval = returnError(tx_data->aun.data, tx_length, 0x000000CF);
							break;
					}
				}
				break;

			// &41: Read/write system information
			case 0x41 :
				if (rx_length > 14) {
					switch (rx_data->aun.data[0x05]) {
						// &00: Reset print server information
						case 0x00 :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &01: Read current state of printer
						case 0x01 :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &02: Write current state of printer
						case 0x02 :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &03: Read auto printer priority
						case 0x03 :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &04: Write auto printer priority
						case 0x04 :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &05: Read system message channel
						case 0x05 :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &06: Write system message channel
						case 0x06 :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &07: Read message level
						case 0x07 :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &08: Write message level
						case 0x08 :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &09: Read default printer
						case 0x09 :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &0A: Write default printer
						case 0x0A :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &0B: Read the privilege required to change the file servers date and time
						case 0x0B :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						// &0C: Set the privilege required to change the file servers date and time
						case 0x0C :
							tx_data->aun.data[0x00] = 0x00;					// Command
							tx_data->aun.data[0x01] = 0x00;					// Error code

							retval = 2;
							break;

						default :
							retval = returnError(tx_data->aun.data, tx_length, 0x000000CF);
							break;
					}
				}
				break;

			default :
				break;
		}

		return retval;
	}

	// &9F PrinterServerEnquiry (See http://mdfs.net/Docs/Books/SJMDFS/Chapter10 / SJR_HDFSSysMgrManual.pdf $7.4)
	int port9Fhandler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length) {
		int retval;

		if ((rx_length < 9) && (tx_length < 12))
			return 0;

		retval = 0;
		switch (rx_data->aun.data[0x06]) {
			// List print servers
			case 0x01 :
				tx_data->aun.port = 0x9E;			// Set port number to FindServerReply
				tx_data->aun.data[0x00] = printer_status;	// Printer status report (&00=Ready, &01=Busy with station nn.ss, &02-&FF=Jammed/Offline
				tx_data->aun.data[0x01] = printer_station;	// Station id
				tx_data->aun.data[0x02] = printer_network;	// Network id

				retval = 1;
				break;

			// List print servers
			case 0x06 :
				tx_data->aun.port = 0x9E;				// Set port number to FindServerReply
				tx_data->aun.data[0x00] = printer_status;	// Printer status report (&00=Ready, &01=Busy with station nn.ss, &02-&FF=Jammed/Offline
				tx_data->aun.data[0x01] = printer_station;	// Station id
				tx_data->aun.data[0x02] = printer_network;	// Network id

				retval = 1;
				break;

			default :
				fprintf(stderr, "Unknown PrintServerEnquiry command received: %02X\n", rx_data->aun.data[0x06]);
				break;
		}

		return retval;
	}

	/* &B0 FindServer */
	int portB0handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length) {
		int i, retval;

		retval = 0;
		if (tx_length > 0x12 + strlen((const char *)settings::servername)) {
			switch (rx_data->aun.control) {
				// &80 FindServer
				case 0x80 :
					if (rx_length == 0x0E) {
						/* Check if the requested servername matches wildcards (8 spaces) or our own servername */
						for (i = 0; i < 8; i++) {
							if ((rx_data->aun.data[i] != 0x20) && (rx_data->aun.data[i] != settings::servername[i]))
								break;
						}
						tx_data->aun.data[0x05] = 0xB1;	// Set port number to FindServerReply
						tx_data->aun.data[0x06] = 0x00;	// Success
						tx_data->aun.data[0x07] = 0xff;	// Further communications on this port number please
						tx_data->aun.data[0x08] = ((atoi(FILESTORE_VERSION_MAJOR) << 4) | (atoi(FILESTORE_VERSION_MINOR) & 0x0F));
						memcpy(&tx_data->aun.data[0x09], ECONET_SERVERTYPE, 8);	// Type of server
						tx_data->aun.data[0x11] = strlen((const char *)settings::servername);
						memcpy(&tx_data->aun.data[0x12], settings::servername, 8);
						retval = (0x12 + tx_data->aun.data[0x11]);
					} else {
						tx_data->aun.data[0x05] = 0xB1;	// Set port number to FindServerReply
						tx_data->aun.data[0x06] = 0xff;	// Error
						tx_data->aun.data[0x07] = 0xff;	// Further communications on this port number please
						tx_data->aun.data[0x08] = ((atoi(FILESTORE_VERSION_MAJOR) << 4) | (atoi(FILESTORE_VERSION_MINOR) & 0x0F));
						memcpy(&tx_data->aun.data[0x09], ECONET_SERVERTYPE, 8);	// Type of server
						tx_data->aun.data[0x11] = strlen((const char *)settings::servername);
						memcpy(&tx_data->aun.data[0x12], settings::servername, 8);
						retval = (0x12 + tx_data->aun.data[0x11]);
					}
					break;
			}
		} else {
			return -1;
		}

		return retval;
	}

	// &D0 PrintServerReply
	int portD0handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length) {
		int retval;

		retval = 0;
		if ((rx_length > 8) && (tx_length > 10)) {
			tx_data->aun.data[0x00] = printer_status;				// Printer status report (&00=Ready, &01=Busy with station nn.ss, &02=Offline
			tx_data->aun.data[0x01] = 0x00;
			retval = 2;
		}

		return retval;
	}

	// &D1 PrintServerData
	int portD1handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length) {
		int i, retval;
char pbuffer[65536];

		if ((rx_length < 9) && (tx_length < 9))
			return 0;

		if (rx_length == 9) {
			switch (rx_data->aun.data[0x00]) {
				// Last bytes have been received, so finish the print job
				case 0x03 :
					fprintf(stderr, "Print job finished\n");
					printer_status = ECONET_PRINTER_READY;
					printer_network = 0;
					printer_station = 0;
					i=ftell (fp_printbuffer);
printf("ftell done\n");
					rewind(fp_printbuffer);
printf("rewind done\n");
					fread(pbuffer, sizeof(uint8_t), i-1, fp_printbuffer);					
printf("fread done\n");
					if (fclose(fp_printbuffer) == -1) {
						perror("pclose failed with");
					}
printf("fclose done\n");
					fp_printbuffer = NULL;
printf("fp_printbuffer done\n");
					print(pbuffer);
printf("print done\n");
					break;

				// Start a new print job
				case 0x05 :
					fprintf(stderr, "Print job started\n");
					if ((fp_printbuffer = fopen(PRINTBUFFERFILE, "w")) == NULL) {
						fprintf(stderr, "portD1handler: Unable to open printer\n");
						fp_printbuffer = NULL;
					} else {
						printer_status = ECONET_PRINTER_BUSY;
						printer_network = 0xFF;
						printer_station = 0xFF;
					}
					break;

				// Unknown PrintServer command received
				default :
					fprintf(stderr, "Unknown print job command received: %02X\n", rx_data->aun.data[0x00]);
					break;
			}
		} else {
			// Check if a print job is running
			if (fp_printbuffer != NULL) {
				// If so, append the received data to the print buffer
				fwrite(rx_data->aun.data, sizeof(uint8_t), rx_length - 8, fp_printbuffer);
			}
		}

		tx_data->aun.port = 0xD1;				// Set port number to FindServerReply
		tx_data->aun.control = 0x00;
		tx_data->aun.data[0x00] = 0x80;
		tx_data->aun.data[0x01] = 0x00;
		tx_data->aun.data[0x02] = 0x01;
		retval = 1;
		return retval;
	}

	/* Return an error */
	int returnError(uint8_t *tx_data, size_t tx_length, uint32_t errorNumber) {
		int len;

		len = strlen(getErrorString(errorNumber));
		tx_data[0] = 0;
		tx_data[1] = errorNumber & 0xFF;
		strlcpy((char *)&tx_data[2], getErrorString(errorNumber), tx_length-2);
		tx_data[2 + len] = 0x0D;
		return (3 + len);
	}

}

