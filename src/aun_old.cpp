/* ethernet.cpp
 * All Ethernet send and receive functions
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstring>			// Included for memset() and memcpy()
#include <sys/socket.h>			// Included for SO_RCVTIMEO
#include <netinet/in.h>			// Included for IPPROTO_UDP
#include <arpa/inet.h>			// Included for inet_aton()
#include "econet.h"			// Included for Frame
#include "ethernet.h"			// Header file for this code
#include "main.h"			// Included for bye variable

using namespace std;



namespace ethernet {
	/* Periodically check if we've received an Econet network package */
	void pollAUNNetworkReceive(void) {
		int reuseconn;
		int rx_sock;
		int rx_length;
		char *buf[sizeof(econet::Frame)];
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
         
			//try to receive some data, this is a blocking call
			if ((rx_length = recvfrom(rx_sock, buf, sizeof(econet::Frame), 0, (struct sockaddr *) &addr_incoming, &slen)) > 0) {
				econet::checkValidFrame((econet::Frame *) &buf, rx_length);
				if (econet::netmon == true) {
					econet::netmonPrintFrame("eth", false, (econet::Frame *) &buf, rx_length);
				}
			}
//			} else
//				usleep(10000);
		}
	}

	int transmitAUNFrame(econet::Frame *frame, int tx_length) {
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
			econet::netmonPrintFrame("eth", true, frame, tx_length);

		if (sendto(tx_sock, (char *) &frame, sizeof(frame), 0, (struct sockaddr *) &addr_outgoing, slen) == -1) {
			fprintf(stderr, "sendto() failed\n");
			return(-3);
		}
//		close(tx_sock);
		return(0);
	}
}

