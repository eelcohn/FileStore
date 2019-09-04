/* aun.cpp
 * All AUN (Acorn Universal Networking) send and receive functions
 *
 * (c) Eelco Huininga 2017-2019
 */

#include <cstdlib>		// strtol()
#include <cstring>		// memset() and memcpy()
#include <unistd.h>		// close()

#include "aun.h"		// Header file for this code
#include "cli.h"		// netmonPrintFrame()
#include "econet.h"		// econet::Frame
#include "errorhandler.h"	// errorHandler::errorMessages[]
#include "main.h"		// Included for bye variable
#include "netfs.h"
#include "settings.h"		// Global configuration variables are defined here
#include "stations.h"		// stations::stations[][]
#if (FILESTORE_WITHOPENSSL == 1)
#include "dtls/dtls.h"		// All functions for DTLS (Datagram TLS)
#endif

using namespace std;



namespace aun {
	char straddr[INET6_ADDRSTRLEN];

	int transmitFrame(econet::Frame *frame, unsigned int tx_length) {
		char ipstr[256];
		int n, s;

		for (n = 1; n < 127; n++) {
			for (s = 1; s < 255; s++) {
				switch (stations::stations[n][s].type) {
					case STATION_IPV4 :
						inet_ntop(AF_INET, &stations::stations[n][s].ipv4, ipstr, sizeof(ipstr));
						if (strlen(stations::stations[n][s].fingerprint) == 0) {
							aun::ipv4_aun_Transmit(ipstr, stations::stations[n][s].port, frame, tx_length);
#if (FILESTORE_WITHOPENSSL == 1)
						} else {
							aun::ipv4_dtls_Transmit(ipstr, stations::stations[n][s].port, frame, tx_length);
#endif
						}
						break;

#if (FILESTORE_WITHIPV6 == 1)
					case STATION_IPV6 :
						inet_ntop(AF_INET6, &stations::stations[n][s].ipv6, ipstr, sizeof(ipstr));
						if (strlen(stations::stations[n][s].fingerprint) == 0) {
							aun::ipv6_aun_Transmit(ipstr, stations::stations[n][s].port, frame, tx_length);
#if (FILESTORE_WITHOPENSSL == 1)
						} else {
							aun::ipv6_dtls_Transmit(ipstr, stations::stations[n][s].port, frame, tx_length);
#endif
						}
						break;
#endif
					default :
						break;
				}
			}
		}

		return 0;
	}

	/* Periodically check if we've received an Econet network package */
	int ipv4_aun_Listener(void) {
		econet::Frame rx_data, tx_data, ack;
		int rx_length, tx_length;
		int reuseconn;
		int rx_sock;
		bool sendAck;

		struct sockaddr_in addr_me, addr_incoming;
		struct timeval timeout;
		socklen_t slen = sizeof(addr_incoming);

		if ((rx_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			fprintf(stderr, "aun::ipv4_aun_Listener: socket() failed.\n");
			return -1;
		}

		/* Set socket to allow multiple connections */
		reuseconn = 1;
		if (setsockopt(rx_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseconn, sizeof(reuseconn)) == -1) {
			fprintf(stderr, "aun::ipv4_aun_Listener: setsockopt(SO_REUSEADDR).\n");
			return -1;
		}

		/* Set timeout on socket to prevent recvfrom from blocking execution */
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
		if (setsockopt(rx_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
			fprintf(stderr, "aun::ipv4_aun_Listener: Error setting timeout on socket.\n");
			return -1;
		}

		/* Set IP header */
		bzero(&addr_me, sizeof(addr_me));
//		memset((char *) &addr_me, 0, sizeof(addr_me));
		addr_me.sin_family	= AF_INET;
		addr_me.sin_port	= htons(settings::aun_port);
		addr_me.sin_addr.s_addr	= htonl(INADDR_ANY);

		/* Bind to the socket */
		if (bind(rx_sock, (struct sockaddr *) &addr_me, sizeof(addr_me)) == -1) {
			fprintf(stderr, "aun::ipv4_aun_Listener: Error on bind.\n");
			return -1;
		}

		printf("- Listening for UDP4 connections on %s:%i\n", inet_ntoa(addr_me.sin_addr), settings::aun_port);
		fflush(stdout);
		while (bye == false) {
			if ((rx_length = recvfrom(rx_sock, (econet::Frame *) &rx_data, sizeof(rx_data), 0, (struct sockaddr *) &addr_incoming, &slen)) > 0) {
				if (econet::netmon == true) {
					netmonPrintFrame("eth", false, &rx_data, rx_length);
				}
				tx_length = rxHandler(&rx_data, rx_length, &tx_data, sizeof(tx_data), &sendAck);
				if (sendAck) {
					if ((prepareAckPackage(&rx_data, rx_length, &ack, sizeof(ack))) > 0) {
						if (sendto(rx_sock, (char *) &ack, 8, 0, (struct sockaddr *) &addr_incoming, slen) == -1) {
							fprintf(stderr, "aun::ipv4_aun_Listener: sendto() ACK failed.\n");
						}
					} else {
						fprintf(stderr, "aun::ipv4_aun_Listener: prepareAckPackage() failed.\n");
					}
				}
				if (tx_length > 0) {
					if (econet::netmon == true) {
						netmonPrintFrame("eth", true, &tx_data, tx_length);
					}
					if (sendto(rx_sock, (char *) &tx_data, tx_length, 0, (struct sockaddr *) &addr_incoming, slen) == -1) {
						fprintf(stderr, "aun::ipv4_aun_Listener: sendto() data failed.\n");
					} // TODO: Wait for receive ack, or open session which closes on receive ack
				}
			} else {
				/* Ease down on the CPU when polling the network */
//				usleep(10000);
			}
		}
		printf("- Listener stopped on %s:%i\n", inet_ntoa(addr_me.sin_addr), settings::aun_port);
		return 0;
	}

	int ipv4_aun_Transmit(const char *address, unsigned short port, econet::Frame *frame, size_t tx_length) {
		struct sockaddr_in addr_outgoing;
		int tx_sock, slen = sizeof(addr_outgoing);
 
		if ((tx_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			fprintf(stderr, "aun::ipv4_aun_Transmit: socket() failed.\n");
			return(-1);
		}
 
		bzero(&addr_outgoing, sizeof(addr_outgoing));
//		memset((char *) &addr_outgoing, 0, sizeof(addr_outgoing));
		addr_outgoing.sin_family	= AF_INET;
		addr_outgoing.sin_port		= htons(port);
     
		if (inet_pton(AF_INET, address, &addr_outgoing.sin_addr) == 0) {
			fprintf(stderr, "aun::ipv4_aun_Transmit: invalid IPv4 address.\n");
			return(-2);
		}

		if (econet::netmon)
			netmonPrintFrame("eth", true, frame, tx_length);

		if (sendto(tx_sock, (char *) &frame, sizeof(frame), 0, (struct sockaddr *) &addr_outgoing, slen) == -1) {
			fprintf(stderr, "aun::ipv4_aun_Transmit: sendto() failed.\n");
			return(-3);
		}
//		close(tx_sock);
		return(0);
	}

#if (FILESTORE_WITHOPENSSL == 1)
	/* Periodically check if we've received an Econet network package */
	int ipv4_dtls_Listener(void) {
		DTLSParams	server;				/* All variables and objects for this DTLS connection */

		/* Initialize SSL Engine and context */
		server.type		= DTLS_SERVER;			/* Initialize an OpenSSL server context */
		server.family		= AF_INET;			/* IPv4 connection */
		server.port		= htons(settings::dtls_port);	/* UDP port number */
		server.ciphers		= "ALL:kECDHE:!COMPLEMENTOFDEFAULT:!EXPORT:!EXP:!LOW:!MD5:!aNULL:!eNULL:!SSLv2:!SSLv3:!TLSv1:!ADH:!kRSA:!SHA1";
		server.sigalgs		= "ECDSA+SHA512:RSA+SHA512";
		server.dhfile		= "./dh4096.pem";
		server.ca		= NULL;
		server.cert		= "./server-cert.pem";
		server.privkey		= "./server-key.pem";
		server.rxhandler	= aun::rxHandler;		/* Handler for received data */

		if (dtls::ssl_initialize(&server) != 0) {
			fprintf(stderr, "aun::ipv4_dtls_Listener: dtls::ssl_initialize() failed\n");
			return -1;
		}

		/* Start DTLS server loop */
		printf("- Listening for UDP4 DTLS connections on %s:%i\n", server.address, settings::dtls_port); 
		fflush(stdout);
		dtls::server(&server);

		printf("- Listener stopped on %s:%i\n", server.address, settings::dtls_port);
		return 0;
	}

	int ipv4_dtls_Transmit(const char *address, unsigned short port, econet::Frame *frame, size_t tx_length) {
		DTLSParams client;
		char buffer[256];
		size_t rx_length;
		int read, i;

		/* Initialize whatever OpenSSL state is necessary to execute the DTLS protocol. */
		dtls::start();

		/* Initialize the DTLS context from the keystore and then create the server SSL state */
		if (dtls::initContextFromKeystore(&client, "client", AF_INET, address, port) < 0) {
			fprintf(stderr, "aun::ipv4_dtls_Transmit: dtls_InitContextFromKeystore() error.\n");
			dtls::shutdown(&client);
			return -1;
		}

		/* Open a DTLS connection to server:port */
		sprintf(buffer, "%s:%d", address, port);
		if (dtls::initClient(&client, buffer) < 0) {
			fprintf(stderr, "aun::ipv4_dtls_Transmit: dtls_InitClient() error.\n");
			dtls::shutdown(&client);
			return -1;
		}

		// Attempt to connect to the server and complete the handshake.
		int result = SSL_connect(client.ssl);
		if (result != 1) {
			fprintf(stderr, "aun::ipv4_dtls_Transmit: Unable to connect to %s:%d.\n", address, port);
			dtls::shutdown(&client);
			return -1;
		}

		// Write the buffer to the server
		if (econet::netmon)
			netmonPrintFrame("eth", true, frame, tx_length);

		unsigned int written = SSL_write(client.ssl, frame->data, tx_length);
		if (written != tx_length) {
			fprintf(stderr, "aun::ipv4_dtls_Transmit: %i of %lu written.\n", written, tx_length);
			dtls::shutdown(&client);
			return -2;
		}

		rx_length = 0;
		read = -1;
		do {
			// Read the output from the server. If it's not empty, print it.
			read = SSL_read(client.ssl, buffer, sizeof(buffer));
			if (read > 0) {
				printf("IN[%d]: ", read);
				for (i = 0; i < read; i++) {
					printf("%c", buffer[i]);
				}
				printf("\n");
				rx_length += read;
			}
		} while (read < 0);

		// Teardown the link and context state.
		dtls::shutdown(&client);

		return(rx_length);
	}
#endif
#if (FILESTORE_WITHIPV6 == 1)
	/* Periodically check if we've received an Econet network package */
	int ipv6_aun_Listener(void) {
		int reuseconn;
		int rx_sock;
		int rx_length;
		econet::Frame frame;
		bool valid;

		struct sockaddr_in6 addr_me, addr_incoming;
		struct timeval timeout;
		socklen_t slen = sizeof(addr_incoming);

		if ((rx_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			fprintf(stderr, "aun::ipv6_aun_Listener: socket() failed.\n");
			return -1;
		}

		/* Set socket to allow multiple connections */
		reuseconn = 1;
		if (setsockopt(rx_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseconn, sizeof(reuseconn)) == -1) {
			fprintf(stderr, "aun::ipv6_aun_Listener: setsockopt(SO_REUSEADDR).\n");
			return -1;
		}

		/* Set timeout on socket to prevent recvfrom from blocking execution */
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
		if (setsockopt(rx_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
			fprintf(stderr, "aun::ipv6_aun_Listener: Error setting timeout on socket.\n");
			return -1;
		}

		/* Set IP header */
		bzero(&addr_me, sizeof(addr_me));
//		memset((char *) &addr_me, 0, sizeof(addr_me));
		addr_me.sin6_family	= AF_INET6;
		addr_me.sin6_port	= htons(settings::aun_port);
		addr_me.sin6_addr	= in6addr_any;

		/* Bind to the socket */
		if (bind(rx_sock, (struct sockaddr *) &addr_me, sizeof(addr_me)) == -1) {
			fprintf(stderr, "aun::ipv6_aun_Listener: Error on bind.\n");
			return -1;
		}

		printf("- Listening for UDP6 connections on [%s]:%i\n", inet_ntop(AF_INET6, &addr_me.sin6_addr, straddr, sizeof(straddr)), settings::aun_port); 
		fflush(stdout);
		while (bye == false) {
			if ((rx_length = recvfrom(rx_sock, (econet::Frame *) &frame, sizeof(frame), 0, (struct sockaddr *) &addr_incoming, &slen)) > 0) {
				valid = econet::validateFrame(&frame, rx_length);
				if (econet::netmon == true) {
					netmonPrintFrame("eth", false, &frame, rx_length);
				}
				if (valid) {
					if (frame.flags || ECONET_FRAME_TOLOCAL) {
						/* Frame is addressed to a station on our local network */
						if (frame.flags || ECONET_FRAME_TOME) {
							/* Frame is addressed to us */
							econet::processFrame(&frame, rx_length);
						} else {
							/* Frame is addressed to a station on our local network */
//							frame.data[0] = 0x00; // Set destination network to local network
							econet::transmitFrame((econet::Frame *) &frame, rx_length);
						}
					} else {
						/* Frame is addressed to a station on another network */
//						if ((configuration::relay_only_known_networks) && (econet::known_networks[frame.dst_network].network == 0)) {
							/* Don't relay the frame, but reply with ICMP 3.0: Destination network unknown */
//							ethernet::send ICMP 3.0: Destination network unknown
//						} else {
							/* Relay frame to other known network(s) */
							econet::transmitFrame((econet::Frame *) &frame, rx_length);
//						}
					}
				}
			} else {
				/* Ease down on the CPU when polling the network */
//				usleep(10000);
			}
		}
		printf("- Listener stopped on [%s]:%i\n", inet_ntop(AF_INET6, &addr_me.sin6_addr, straddr, sizeof(straddr)), settings::aun_port);
		return 0;
	}

	int ipv6_aun_Transmit(const char *address, unsigned short port, econet::Frame *frame, size_t tx_length) {
		struct sockaddr_in6 addr_outgoing;
		int tx_sock, slen = sizeof(addr_outgoing);
 
		if ((tx_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			fprintf(stderr, "aun::ipv6_aun_Transmit: socket() failed.\n");
			return(-1);
		}
 
		bzero(&addr_outgoing, sizeof(addr_outgoing));
//		memset((char *) &addr_outgoing, 0, sizeof(addr_outgoing));
		addr_outgoing.sin6_family	= AF_INET6;
		addr_outgoing.sin6_port		= htons(port);
     
		if (inet_pton(AF_INET6, address, &addr_outgoing.sin6_addr) == 0) {
			fprintf(stderr, "aun::ipv6_aun_Transmit: invalid IPv6 address.\n");
			return(-2);
		}

		if (econet::netmon)
			netmonPrintFrame("eth", true, frame, tx_length);

		if (sendto(tx_sock, (char *) &frame, sizeof(frame), 0, (struct sockaddr *) &addr_outgoing, slen) == -1) {
			fprintf(stderr, "aun::ipv6_aun_Transmit: sendto() failed.\n");
			return(-3);
		}
//		close(tx_sock);
		return(0);
	}

#if (FILESTORE_WITHOPENSSL == 1)
	int ipv6_dtls_Listener(void) {
		DTLSParams	server;				/* All variables and objects for this DTLS connection */

		/* Initialize SSL Engine and context */
		server.type		= DTLS_SERVER;			/* Initialize an OpenSSL server context */
		server.family		= AF_INET6;			/* IPv4 connection */
		server.address		= NULL;				/* Address to bind to */
		server.port		= htons(settings::dtls_port);	/* UDP port number */
		server.ciphers		= "ALL:kECDHE:!COMPLEMENTOFDEFAULT:!EXPORT:!EXP:!LOW:!MD5:!aNULL:!eNULL:!SSLv2:!SSLv3:!TLSv1:!ADH:!kRSA:!SHA1";
		server.sigalgs		= "ECDSA+SHA512:RSA+SHA512";
		server.dhfile		= "./dh4096.pem";
		server.ca		= NULL;
		server.cert		= "./server-cert.pem";
		server.privkey		= "./server-key.pem";
		server.rxhandler	= aun::rxHandler;		/* Handler for received data */

		if (dtls::ssl_initialize(&server) != 0) {
			fprintf(stderr, "aun::ipv6_dtls_Listener: dtls::ssl_initialize() failed\n");
			return -1;
		}

		/* Start DTLS server loop */
		printf("- Listening for UDP6 DTLS connections on %s:%i\n", server.address, settings::dtls_port); 
		fflush(stdout);
		dtls::server(&server);

		printf("- Listener stopped on %s:%i\n", server.address, settings::dtls_port);
		return 0;
	}

	int ipv6_dtls_Transmit(const char *address, unsigned short port, econet::Frame *frame, size_t tx_length) {
		char buffer[256];

		/* Place the IPv6 address in brackets and call the IPv4 DTLS Transmitter */
		sprintf(buffer, "[%s]", address);

		return(aun::ipv4_dtls_Transmit(buffer, port, frame, tx_length));
	}
#endif
#endif
	int prepareAckPackage(econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length) {
		if ((rx_length < 8) || (tx_length < 8))
			return -1;

		tx_data->control = 0;
		tx_data->port = 0;
		tx_data->flags = 0;

		memcpy(tx_data, rx_data, 8);
		tx_data->aun.type = AUN_ACK;

		if (econet::netmon == true) {
			netmonPrintFrame("eth", true, (econet::Frame *) tx_data, 8);
		}

		return 8;
	}

	int rxHandler(econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length, bool *sendAck) {
		int result, datalen;

		/* temp stuff to prevent unintialized variables in netmon */
		rx_data->control = 0;
		rx_data->port = 0;
		tx_data->control = 0;
		tx_data->port = 0;

econet::protohandlers[0x00] = econet::port00handler;
econet::protohandlers[0x90] = econet::port90handler;
econet::protohandlers[0x91] = econet::port91handler;
econet::protohandlers[0x99] = econet::port99handler;
econet::protohandlers[0x9F] = econet::port9Fhandler;
econet::protohandlers[0xB0] = econet::portB0handler;
econet::protohandlers[0xD0] = econet::portD0handler;
econet::protohandlers[0xD1] = econet::portD1handler;

		result = 0;
		*sendAck = false;
		if (aun::validateFrame(rx_data, rx_length)) {
			switch (rx_data->aun.type) {
				case AUN_BROADCAST :
				case AUN_UNICAST :
					if (econet::protohandlers[rx_data->aun.port] != NULL) {
						*sendAck = true;
						tx_data->aun.type = AUN_UNICAST;
						tx_data->aun.port = rx_data->aun.replyport;
						tx_data->aun.control = 0; // TODO: Result
						tx_data->aun.retry = 0;
						tx_data->aun.sequence = rx_data->aun.sequence + 1; // This should probably our own sequence number generated by FileStore
						if ((datalen = econet::protohandlers[rx_data->aun.port](rx_data, rx_length, tx_data, tx_length)) > 0) {
							result = 8 + datalen;
						}
					}
					break;

				case AUN_ACK :
					break;

				case AUN_NAK :
					break;

				case AUN_IMMEDIATE :
					datalen = econet::port00handler(rx_data, rx_length, tx_data, tx_length);
					tx_data->aun.type = AUN_IMMEDIATE_REPLY;
					tx_data->aun.port = rx_data->aun.port;
					tx_data->aun.control = rx_data->aun.control;
					tx_data->aun.retry = 0;
					tx_data->aun.sequence = rx_data->aun.sequence;
					result = 8 + datalen;
					break;

				case AUN_IMMEDIATE_REPLY :
					break;

				default :
					fprintf(stderr, "aun::rxHandler: Unknown transaction type 0x%02X in AUN frame\n", rx_data->aun.type);
					break;
			}
		}

		return result;
	}

	/* Check if a frame is a valid Econet frame */
	bool validateFrame(econet::Frame *data, size_t length) {
		data->flags = 0;

		/* Check if there's at least an AUN header available */
		if (length < 8) {
			data->flags |= ECONET_FRAME_INVALID;
			return false;
		}
		/* Check if the transaction type is valid */
		if ((data->aun.type < AUN_BROADCAST) || (data->aun.type > AUN_IMMEDIATE_REPLY)) {
			data->flags |= ECONET_FRAME_INVALID;
			return false;
		}

		return true;
	}
}
