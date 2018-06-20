/* econet.cpp
 * All Econet send and receive functions
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdlib>			// Included for strtol()
#include <cstring>			// Included for memcpy(), strlen()
#include <ctime>			// Included for time(), tm

#include "platforms/platform.h"		// All high-level API calls
#include "settings.h"			// Global configuration variables are defined here
#include "econet.h"			// Header file for this code
#include "ethernet.h"			// Included for ethernet::transmitFrame()
#include "main.h"			// Included for bye variable
#include "commands/commands.h"		// Included for commands::netmonPrintFrame()

using namespace std;



namespace econet {
	Station known_networks[256];
	Session sessions[ECONET_MAX_SESSIONS];
	bool	netmon;

	/* Periodically check if we've received an Econet network package */
	void pollNetworkReceive(void) {
		int rx_length;
		econet::Frame frame;

		while (bye == false) {
			rx_length = api::receiveData(&frame);
			if ((rx_length > 0) && (econet::netmon))
				commands::netmonPrintFrame("eco  ", false, &frame, rx_length);
			if (econet::validateFrame((econet::Frame *) &frame, rx_length)) {
				if (frame.status || ECONET_FRAME_TOLOCAL) {
					/* Frame is addressed to a station on our local network */
					if (frame.status || ECONET_FRAME_TOME) {
						/* Frame is addressed to us */
						econet::processFrame(&frame, rx_length);
					} else {
						/* Frame is addressed to a station on our local network */
						frame.data[0] = 0x00; // Set destination network to local network
						econet::transmitFrame((econet::Frame *) &frame, rx_length);
					}
				} else {
					/* Frame is addressed to a station on another network */
//					if ((configuration::relay_only_known_networks) && (econet::known_networks[frame.dst_network].network == 0)) {
						/* Don't relay the frame, but reply with &3A2 Not listening */
//						econet::transmitFrame((econet::Frame *) &frame, rx_length);
//					} else {
						/* Relay frame to other known network(s) */
						ethernet::transmitFrame(NULL, ETHERNET_AUN_UDPPORT, (econet::Frame *) &frame, rx_length);
//					}
				}
			}
		}
	}

	void transmitFrame(econet::Frame *frame, int size) {
		if (econet::netmon)
			commands::netmonPrintFrame("eco  ", true, frame, size);
//		api::transmit(frame, size);
	}

	/* Check if a frame is a valid Econet frame */
	bool validateFrame(econet::Frame *frame, int size) {
		if (size < 4)
			frame->status |= ECONET_FRAME_INVALID;
		if (size == 4)
			frame->status |= ECONET_FRAME_ACK;
		if (size == 6)
			frame->status |= ECONET_FRAME_SCOUT;
		if (size > 4)
			frame->status |= ECONET_FRAME_DATA;
		if ((frame->dst_network | frame->dst_station) == 0xFF)
			frame->status |= ECONET_FRAME_BROADCAST;
		if ((frame->dst_network == configuration::econet_network) || (frame->dst_network == 0x00)) {
			frame->status |= ECONET_FRAME_TOLOCAL;
			if (frame->dst_station == configuration::econet_station) {
				frame->status |= ECONET_FRAME_TOME;
			}
		}

		if (frame->status || ECONET_FRAME_INVALID) {
			return false;
		} else {
			frame->dst_network = frame->data[0];
			frame->dst_station = frame->data[1];
			frame->src_network = frame->data[2];
			frame->src_station = frame->data[3];
			frame->port = frame->data[4];
			if (econet::hasSession(frame->src_network, frame->src_station, frame->port) == false) {
				/* New session */
				frame->control = frame->data[4];
				frame->port = frame->data[5];
				if ((frame->control & 0x80) != 0x80)
					frame->status |= ECONET_FRAME_INVALID;
			} else {
				/* Resume session on reply port (frame has no control byte) */
				frame->control = 0x00;
			}
			return true;
		}
	}

	/* Processes one Econet frame */
	void processFrame(econet::Frame *frame, int size) {
		int i;
		unsigned char urd_handle, csd_handle, lib_handle;
		unsigned char first_drive, num_drives;

		frame->data[0] = frame->src_network;
		frame->data[1] = frame->src_station;
		frame->data[2] = configuration::econet_network;
		frame->data[3] = configuration::econet_station;

		switch (frame->port) {
			// &00 Immediate
			case 0x00 :
				switch (frame->control) {
					// &01 Received text message
					case 0x01 :
						break;

					// &02 Show error
					case 0x02 :
						break;

					// &80 OSCLI
					case 0x80 :
						break;

					// &81 PEEK
					case 0x81 :
						break;

					// &82 POKE
					case 0x82 :
						break;

					// &83 JSR
					case 0x83 :
						break;

					// &84 USERPROC
					case 0x84 :
						break;

					// &85 OSPROC
					case 0x85 :
						break;

					// &86 HALT
					case 0x86 :
						break;

					// &87 CONTINUE
					case 0x87 :
						break;

					// &88 MACHINETYPE
					case 0x88 :
						frame->data[0x06] = ECONET_MACHINETYPE >> 8;		// Machine type MSB
						frame->data[0x07] = ECONET_MACHINETYPE && 0xff;		// Machine type LSB
						econet::transmitFrame(frame, 8);
						break;

					// &89 GETREGISTERS
					case 0x89 :
						break;

					default :
						break;
				}
				break;

			// &90 FileServerReply
			case 0x90 :
				urd_handle = frame->data[6];
				csd_handle = frame->data[7];
				lib_handle = frame->data[8];
				frame->data[4] = frame->data[5];		// Prepare reply frame

				// Check the OSWORD &14 function code at XY+00
				switch (frame->data[5]) {
					// &00: Command line decoding
					case 0x00 :
						break;

					// &01: Save
					case 0x01 :
						break;

					// &02: Load
					case 0x02 :
						break;

					// &03: Examine
					case 0x03 :
						break;

					// &04: Read catalogue header
					case 0x04 :
						break;

					// &05: Load as command
					case 0x05 :
						break;

					// &06: Open file
					case 0x06 :
						break;

					// &07: Close file
					case 0x07 :
						break;

					// &08: Get byte
					case 0x08 :
						break;

					// &09: Put byte
					case 0x09 :
						break;

					// &0A: Get multiple bytes
					case 0x0A :
						break;

					// &0B: Put multiple bytes
					case 0x0B :
						break;

					// &0C: Read random access information
					case 0x0C :
						break;

					// &0D: Set random access information
					case 0x0D :
						break;

					// &0E: Read disc name information
					case 0x0E :
						frame->data[5] = 0x00;	// Return code (0 = success)
						first_drive = frame->data[8];
						num_drives = frame->data[9];
						for (i = first_drive; i < ECONET_MAX_DISCDRIVES; i++) {
							frame->data[6] = i;
							strncpy((char *)&frame->data[7 + ((i - first_drive) * 16)], (const char *)discs[i].title, 16);
						}
						break;

					// &0F: Read logged on users
					case 0x0F :
						break;

					// &10: Read date/time
					case 0x10 :
						struct tm timeinfo;
						frame->data[5] = 0x00;	// Return code (0 = success)
						frame->data[6] = timeinfo.tm_mday;
						frame->data[7] = ((timeinfo.tm_year & 0x0F) << 4) || timeinfo.tm_mon;
						frame->data[8] = timeinfo.tm_hour;
						frame->data[9] = timeinfo.tm_min;
						frame->data[10] = timeinfo.tm_sec;
						break;

					// &11: Read EOF (End Of File) information
					case 0x11 :
						break;

					// &12: Read object information
					case 0x12 :
						break;

					// &13: Set object information
					case 0x13 :
						break;

					// &14: Delete object
					case 0x14 :
						break;

					// &15: Read user environment
					case 0x15 :
						break;

					// &16: Set user's boot option
					case 0x16 :
						break;

					// &17: Log off
					case 0x17 :
						break;

					// &18: Read user information
					case 0x18 :
						break;

					// &19: Read file server version number
					case 0x19 :
						sprintf((char *)&frame->data[5], "v%s.%s.%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCHLEVEL);
						break;

					// &1A: Read file server free space
					case 0x1A :
						break;

					// &1B: Create directory
					case 0x1B :
						break;

					// &1C: Set date/time
					case 0x1C :
						break;

					// &1D: Create file of specified size
					case 0x1D :
						break;

					// &1E: Read user free space
					case 0x1E :
						break;

					// &1F: Set user free space
					case 0x1F :
						break;

					// &20: Read client user identifier
					case 0x20 :
						break;

					// &40: Read account information
					case 0x40 :
						break;

					// &41: Read/write system information
					case 0x41 :
						break;

					default :
						break;
				}
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
						econet::startSession(frame->src_network, frame->src_station, 0x90);	// Start a session with this client
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
						if (configuration::econet_network == frame->src_network) {
							printf("Warning! Found another Econet network with the same network number as ours! Please check the configuration at station %i:%i\n", frame->src_network, frame->src_station);
						} else {
							// Add the new Econet network to our list of known networks
							econet::known_networks[frame->src_network].network = frame->src_network;
							econet::known_networks[frame->src_network].station = frame->src_station;
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
				switch (frame->control) {
					case 0x80 :
						if (size == 0x0E) {
							/* Check if the requested servername matches wildcards (8 spaces) or our own servername */
							for (i = 0; i < 6; i++) {
								if ((frame->data[i] != 0x20) && (frame->data[i] != configuration::printername[i]))
									break;
							}
							frame->data[0x05] = 0x9E;	// Set port number to FindServerReply
							frame->data[0x06] = 0x00;	// Printer status report (0=ready, 1=busy with station nn:ss, 2=offline)
							frame->data[0x07] = 0x00;	// Station id
							frame->data[0x08] = 0x00;	// Station id
							econet::transmitFrame(frame, (0x0C + frame->data[0x0B]));	// Send reply back to client
						} else {
							frame->data[0x05] = 0x9E;	// Set port number to FindServerReply
							frame->data[0x06] = 0x00;	// Printer status report
							frame->data[0x07] = 0x00;	// Station id
							frame->data[0x08] = 0x00;	// Station id
							econet::transmitFrame(frame, (0x0C + frame->data[0x0B] + 17));	// Send reply back to client
						}
						break;

					default :
						break;
				}
				break;

			// &B0 FindServer
			case 0xB0 :
				switch (frame->control) {
					// &80 FindServer
					case 0x80 :
						if (size == 0x0E) {
							/* Check if the requested servername matches wildcards (8 spaces) or our own servername */
							for (i = 0; i < 8; i++) {
								if ((frame->data[i] != 0x20) && (frame->data[i] != configuration::servername[i]))
									break;
							}
							frame->data[0x05] = 0xB1;	// Set port number to FindServerReply
							frame->data[0x06] = 0x00;	// Success
							frame->data[0x07] = 0xff;	// Further communications on this port number please
							frame->data[0x08] = ((atoi(VERSION_MAJOR) << 4) | (atoi(VERSION_MINOR) & 0x0F));
							memcpy(&frame->data[0x09], ECONET_SERVERTYPE, 8);	// Type of server
							frame->data[0x11] = strlen((const char *)configuration::servername);
							memcpy(&frame->data[0x12], configuration::servername, frame->data[0x0B]);
							econet::transmitFrame(frame, (0x12 + frame->data[0x11]));	// Send reply back to client
						} else {
							frame->data[0x05] = 0xB1;	// Set port number to FindServerReply
							frame->data[0x06] = 0xff;	// Error
							frame->data[0x07] = 0xff;	// Further communications on this port number please
							frame->data[0x08] = ((atoi(VERSION_MAJOR) << 4) | (atoi(VERSION_MINOR) & 0x0F));
							memcpy(&frame->data[0x09], ECONET_SERVERTYPE, 8);	// Type of server
							frame->data[0x11] = strlen((const char *)configuration::servername);
							memcpy(&frame->data[0x12], configuration::servername, frame->data[0x0B]);
							econet::transmitFrame(frame, (0x12 + frame->data[0x11]));	// Send reply back to client
							memcpy(&frame->data[0x12 + frame->data[0x11]], "Wrong frame size", 16);
							econet::transmitFrame(frame, (0x12 + frame->data[0x11] + 17));	// Send reply back to client
						}
						break;
				}
				break;

			// &B1 FindServerReply
			case 0xB1 :
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
					// &81 IP
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

			default:
				break;
		}
	}

	/* Send a broadcast frame to announce that a new bridge is available */
	void sendBridgeAnnounce(void) {
		econet::Frame frame;

		memmove((econet::Frame *) &frame, ECONET_BROADCAST_NEWBRIDGE, sizeof(ECONET_BROADCAST_NEWBRIDGE));
		frame.data[0x02]	= configuration::econet_network;
		frame.data[0x03]	= configuration::econet_station;
		frame.data[0x06]	= configuration::ethernet_network;

		econet::transmitFrame(&frame, sizeof(ECONET_BROADCAST_NEWBRIDGE));

		frame.data[0x02]	= configuration::ethernet_network;
		frame.data[0x03]	= configuration::ethernet_station;
		frame.data[0x06]	= configuration::econet_network;

		ethernet::transmitFrame((char *)"127.0.0.1", ETHERNET_AUN_UDPPORT, &frame, sizeof(ECONET_BROADCAST_NEWBRIDGE));
	#ifdef ECONET_WITHOPENSSL
		ethernet::transmit_dtlsFrame((char *)"127.0.0.1", ETHERNET_SAUN_UDPPORT, &frame, sizeof(ECONET_BROADCAST_NEWBRIDGE));
	#endif
	}

	/* Send a broadcast frame to query other bridges what networks are available */
	void sendWhatNetBroadcast(void) {
		econet::Frame frame;

		memmove((econet::Frame *) &frame, ECONET_BROADCAST_WHATNET, sizeof(ECONET_BROADCAST_WHATNET));
		frame.data[0x02]	= configuration::econet_network;
		frame.data[0x03]	= configuration::econet_station;

		econet::transmitFrame(&frame, sizeof(ECONET_BROADCAST_WHATNET));

		frame.data[0x02]	= configuration::ethernet_network;
		frame.data[0x03]	= configuration::ethernet_station;

		ethernet::transmitFrame((char *)"127.0.0.1", ETHERNET_AUN_UDPPORT, &frame, sizeof(ECONET_BROADCAST_WHATNET));
	#ifdef ECONET_WITHOPENSSL
		ethernet::transmit_dtlsFrame((char *)"127.0.0.1", ETHERNET_SAUN_UDPPORT, &frame, sizeof(ECONET_BROADCAST_WHATNET));
	#endif
	}

	/* Start a session with a client */
	bool startSession(unsigned char network, unsigned char station, unsigned char port) {
		int i;

		i = 0;
		while (i < ECONET_MAX_SESSIONS) {
			if ((sessions[i].network == 0x00) && (sessions[i].station == 0x00) && (sessions[i].port == 0x00)) {
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
	bool hasSession(unsigned char network, unsigned char station, unsigned char port) {
		int i;

		i = 0;
		while (i < ECONET_MAX_SESSIONS) {
			if ((sessions[i].network == network) && (sessions[i].station == station) && (sessions[i].port == port)) {
				if (sessions[i].timeout < (unsigned long)time(NULL)) {
					return true;
				} else {
					/* Session has timed out */
					endSession(network, station, port);
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
	bool endSession(unsigned char network, unsigned char station, unsigned char port) {
		int i;

		i = 0;
		while (i < ECONET_MAX_SESSIONS) {
			if ((sessions[i].network == network) && (sessions[i].station == station) && (sessions[i].port == port)) {
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
}

