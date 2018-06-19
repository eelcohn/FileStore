/* ethernet.cpp
 * All Ethernet send and receive functions
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstring>			// Included for memset() and memcpy()
#include <unistd.h>			// Included for close()
#include <sys/socket.h>			// Included for SO_RCVTIMEO
#include <netinet/in.h>			// Included for IPPROTO_UDP
#include <arpa/inet.h>			// Included for inet_aton()

#include "settings.h"			// Global configuration variables are defined here
#include "dtls.h"
#include "econet.h"			// Included for Frame
#include "ethernet.h"			// Header file for this code
#include "main.h"			// Included for bye variable
#include "commands/commands.h"		// Included for commands::netmonPrintFrame()

using namespace std;



namespace ethernet {
	/* Periodically check if we've received an Econet network package */
	void ipv4_Listener(void) {
		int reuseconn;
		int rx_sock;
		int rx_length;
		econet::Frame frame;

		struct sockaddr_in addr_me, addr_incoming;
		struct timeval timeout;

		if ((rx_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			perror("socket() failed");
		}

		/* Set socket to allow multiple connections */
		reuseconn = 1;
		if (setsockopt(rx_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseconn, sizeof(reuseconn)) == -1) {
			perror("setsockopt(SO_REUSEADDR)");
		}

		/* Set timeout on socket to prevent recvfrom from blocking execution */
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
		if (setsockopt(rx_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
			perror("Error setting timeout on socket");
		}

		/* Set IP header */
		memset((char *) &addr_me, 0, sizeof(addr_me));
		addr_me.sin_family	= AF_INET;
		addr_me.sin_port	= htons(ETHERNET_AUN_UDPPORT);
		addr_me.sin_addr.s_addr	= htonl(INADDR_ANY);
		if (bind(rx_sock, (struct sockaddr *) &addr_me, sizeof(addr_me)) == -1) {
			perror("Error on bind");
		}

		socklen_t slen = sizeof(addr_incoming);
		fflush(stdout);
		while (bye == false) {
			if ((rx_length = recvfrom(rx_sock, (econet::Frame *) &frame, sizeof(econet::Frame), 0, (struct sockaddr *) &addr_incoming, &slen)) > 0) {
				if (econet::netmon == true) {
					commands::netmonPrintFrame("eth", false, &frame, rx_length);
				}
				if (econet::validateFrame(&frame, rx_length)) {
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
	}

	int transmitFrame(econet::Frame *frame, int tx_length) {
		struct sockaddr_in addr_outgoing;
		int tx_sock, slen = sizeof(addr_outgoing);
 
		if ((tx_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			fprintf(stderr, "socket() failed\n");
			return(-1);
		}
 
		memset((char *) &addr_outgoing, 0, sizeof(addr_outgoing));
		addr_outgoing.sin_family	= AF_INET;
		addr_outgoing.sin_port		= htons(ETHERNET_AUN_UDPPORT);
     
		if (inet_aton("127.0.0.1", &addr_outgoing.sin_addr) == 0) {
			fprintf(stderr, "inet_aton() failed\n");
			return(-2);
		}

		if (econet::netmon)
			commands::netmonPrintFrame("eth", true, frame, tx_length);

		if (sendto(tx_sock, (char *) &frame, sizeof(frame), 0, (struct sockaddr *) &addr_outgoing, slen) == -1) {
			fprintf(stderr, "sendto() failed\n");
			return(-3);
		}
//		close(tx_sock);
		return(0);
	}

//#ifdef OPENSSL
	/* Periodically check if we've received an Econet network package */
	void ipv4_dtls_Listener (void) {
		int rx_sock, client, rx_length;
		char buf[4096];
		struct sockaddr_in addr;
		DTLSParams server;

		// Initialize whatever OpenSSL state is necessary to execute the DTLS protocol.
		dtls_Begin();

		// Create the server UDP listener socket
		rx_sock = _createSocket(ETHERNET_SAUN_UDPPORT);

		// Initialize the DTLS context from the keystore and then create the server SSL state.
		if (dtls_InitContextFromKeystore(&server, "server") < 0) {
			exit(EXIT_FAILURE);
		}
		if (dtls_InitServer(&server) < 0) {
			exit(EXIT_FAILURE);
		}

		// Loop forever accepting messages from the client, printing their messages, and then terminating their connections
		while (bye == false) {
			uint len = sizeof(addr);
//			SSL *ssl;

			// Accept an incoming UDP packet (connection)
			client = accept(rx_sock, (struct sockaddr*) &addr, &len);
			if (client < 0) {
//				perror("Unable to accept");
//				exit(EXIT_FAILURE);
			} else {

			// Set the SSL descriptor to that of the client socket
			SSL_set_fd(server.ssl, client);

			// Attempt to complete the DTLS handshake
			// If successful, the DTLS link state is initialized internally
			if (SSL_accept(server.ssl) <= 0) {
				ERR_print_errors_fp(stderr);
			} else {
				// Read from the DTLS link
				if ((rx_length = SSL_read(server.ssl, buf, sizeof(buf))) > 0) {
					econet::validateFrame((econet::Frame *) &buf, rx_length);
					if (econet::netmon == true) {
						commands::netmonPrintFrame("eth s", false, (econet::Frame *) &buf, rx_length);
					}
					// Echo the message back to the client
					SSL_write(server.ssl, "RESPONSE", sizeof("RESPONSE"));
				}
			}

			// When done reading the single message, close the client's connection
			// and continue waiting for another.
			close(client);
}
		}

		// Teardown the link and context state.
		dtls_Shutdown(&server);
	}
//#endif
}

