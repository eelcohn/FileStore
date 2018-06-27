/* aun.cpp
 * All AUN (Acorn Universal Networking) send and receive functions
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstring>		// Included for memset() and memcpy()
#include <unistd.h>		// Included for close()

#include "settings.h"		// Global configuration variables are defined here
#include "dtls.h"
#include "econet.h"		// Included for Frame
#include "aun.h"		// Header file for this code
#include "main.h"		// Included for bye variable
#include "cli.h"		// Included for netmonPrintFrame()

using namespace std;



namespace aun {
	char straddr[INET6_ADDRSTRLEN];

	/* Periodically check if we've received an Econet network package */
	void ipv4_Listener(void) {
		int reuseconn;
		int rx_sock;
		int rx_length;
		econet::Frame frame;
		bool valid;

		struct sockaddr_in addr_me, addr_incoming;
		struct timeval timeout;
		socklen_t slen = sizeof(addr_incoming);

		if ((rx_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
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
		addr_me.sin_port	= htons(AUN_UDPPORT);
		addr_me.sin_addr.s_addr	= htonl(INADDR_ANY);

		/* Bind to the socket */
		if (bind(rx_sock, (struct sockaddr *) &addr_me, sizeof(addr_me)) == -1) {
			perror("Error on bind");
		}

		printf("- Listening for UDP connections on %s:%i\n", inet_ntoa(addr_me.sin_addr), AUN_UDPPORT); 
		fflush(stdout);
		while (bye == false) {
			if ((rx_length = recvfrom(rx_sock, (econet::Frame *) &frame, sizeof(econet::Frame), 0, (struct sockaddr *) &addr_incoming, &slen)) > 0) {
				valid = econet::validateFrame(&frame, rx_length);
				if (econet::netmon == true) {
					netmonPrintFrame("eth", false, &frame, rx_length);
				}
				if (valid) {
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
		printf("- Listener stopped on %s:%i\n", inet_ntoa(addr_me.sin_addr), AUN_UDPPORT);
	}

	int transmitFrame(char *address, unsigned short port, econet::Frame *frame, int tx_length) {
		struct sockaddr_in addr_outgoing;
		int tx_sock, slen = sizeof(addr_outgoing);
 
		if ((tx_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			fprintf(stderr, "aun::transmitFrame: socket() failed\n");
			return(-1);
		}
 
		memset((char *) &addr_outgoing, 0, sizeof(addr_outgoing));
		addr_outgoing.sin_family	= AF_INET;
		addr_outgoing.sin_port		= htons(port);
     
		if (inet_aton(address, &addr_outgoing.sin_addr) == 0) {
			fprintf(stderr, "aun::transmitFrame: invalid IPv4 address\n");
			return(-2);
		}

		if (econet::netmon)
			netmonPrintFrame("eth", true, frame, tx_length);

		if (sendto(tx_sock, (char *) &frame, sizeof(frame), 0, (struct sockaddr *) &addr_outgoing, slen) == -1) {
			fprintf(stderr, "aun::transmitFrame: sendto() failed\n");
			return(-3);
		}
//		close(tx_sock);
		return(0);
	}

#ifdef ECONET_WITHOPENSSL
	/* Periodically check if we've received an Econet network package */
	void ipv4_dtls_Listener (void) {
		int reuseconn;
		int rx_sock;
		int rx_length;
		econet::Frame frame;
		bool valid;

		struct sockaddr_in addr_me, addr_incoming;
		struct timeval timeout;
		socklen_t slen = sizeof(addr_incoming);

		DTLSParams server;
//		SSL *ssl;

		dtls_Begin();

		if ((rx_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
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
		bzero(&addr_me, sizeof(addr_me));
//		memset((char *) &addr_me, 0, sizeof(addr_me));
		addr_me.sin_family	= AF_INET;
		addr_me.sin_port	= htons(DTLS_UDPPORT);
		addr_me.sin_addr.s_addr	= htonl(INADDR_ANY);

		/* Bind to the socket */
		if (bind(rx_sock, (struct sockaddr *) &addr_me, sizeof(addr_me)) != 0) {
			perror("Error on bind");
		}

		/* Start listening to socket */
		if (listen(rx_sock, 10) != 0) {
			perror("Error on listen");
		}

		// Initialize the DTLS context from the keystore and then create the server SSL state.
		if (dtls_InitContextFromKeystore(&server, "server") < 0) {
			exit(EXIT_FAILURE);
		}
		if (dtls_InitServer(&server) < 0) {
			exit(EXIT_FAILURE);
		}

		printf("- Listening for UDP connections on %s:%i\n", inet_ntop(AF_INET, &addr_me.sin_addr, straddr, sizeof(straddr)), DTLS_UDPPORT); 
		fflush(stdout);
		while (bye == false) {
			// Accept an incoming UDP packet (connection)
			int client = accept(rx_sock, (struct sockaddr*) &addr_incoming, &slen);
//			int client = SSL_accept(server.ssl);
			if (client < 0) {
				perror("Unable to accept");
				exit(EXIT_FAILURE);
			}

			// Set the SSL descriptor to that of the client socket
			SSL_set_fd(server.ssl, client);

			// Attempt to complete the DTLS handshake
			// If successful, the DTLS link state is initialized internally
			if (SSL_accept(server.ssl) <= 0) {
				ERR_print_errors_fp(stderr);
			} else {
				// Read from the DTLS link
				if ((rx_length = SSL_read(server.ssl, (econet::Frame *) &frame, sizeof(econet::Frame))) > 0) {
					valid = econet::validateFrame(&frame, rx_length);
					if (econet::netmon == true) {
						netmonPrintFrame("eth", false, &frame, rx_length);
					}
					if (valid) {
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
			// When done reading the single message, close the client's connection and continue waiting for another.
//			close(client);
		}

		// Teardown the link and context state.
		dtls_Shutdown(&server);

		printf("- Listener stopped on %s:%i\n", inet_ntop(AF_INET, &addr_me.sin_addr, straddr, sizeof(straddr)), DTLS_UDPPORT);
	}

	int transmit_dtlsFrame(char *address, unsigned short port, econet::Frame *frame, int tx_length) {
		return(0);
	}
#endif
#ifdef ECONET_WITHIPV6
	/* Periodically check if we've received an Econet network package */
	void ipv6_Listener(void) {
		int reuseconn;
		int rx_sock;
		int rx_length;
		econet::Frame frame;
		bool valid;

		struct sockaddr_in6 addr_me, addr_incoming;
		struct timeval timeout;
		socklen_t slen = sizeof(addr_incoming);

		if ((rx_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
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
		addr_me.sin6_family	= AF_INET6;
		addr_me.sin6_port	= htons(AUN_UDPPORT);
		addr_me.sin6_addr	= in6addr_any;

		/* Bind to the socket */
		if (bind(rx_sock, (struct sockaddr *) &addr_me, sizeof(addr_me)) == -1) {
			perror("Error on bind");
		}

		printf("- Listening for UDP connections on %s:%i\n", inet_ntop(AF_INET6, &addr_me.sin6_addr, straddr, sizeof(straddr)), AUN_UDPPORT); 
		fflush(stdout);
		while (bye == false) {
			if ((rx_length = recvfrom(rx_sock, (econet::Frame *) &frame, sizeof(econet::Frame), 0, (struct sockaddr *) &addr_incoming, &slen)) > 0) {
				valid = econet::validateFrame(&frame, rx_length);
				if (econet::netmon == true) {
					netmonPrintFrame("eth", false, &frame, rx_length);
				}
				if (valid) {
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
		printf("- Listener stopped on %s:%i\n", inet_ntop(AF_INET6, &addr_me.sin6_addr, straddr, sizeof(straddr)), AUN_UDPPORT);
	}
#ifdef ECONET_WITHOPENSSL
	void ipv6_dtls_Listener(void) {
		struct sockaddr_in6 addr_me;

		/* Set IP header */
		memset((char *) &addr_me, 0, sizeof(addr_me));
		addr_me.sin6_family	= AF_INET6;
		addr_me.sin6_port	= htons(DTLS_UDPPORT);
		addr_me.sin6_addr	= in6addr_any;

		printf("- Listening for DTLS connections on %s:%i\n", "[::]", DTLS_UDPPORT);
		while (bye == false) {
//
		}
		printf(" Listener stopped on %s:%i\n", inet_ntop(AF_INET6, &addr_me.sin6_addr, straddr, sizeof(straddr)), DTLS_UDPPORT);
	}
#endif
#endif
}

