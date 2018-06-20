/* ethernet.h
 * All Ethernet send and receive functions
 *
 * (c) Eelco Huininga 2017-2018
 */

#ifndef ECONET_ETHERNET_HEADER
#define ECONET_ETHERNET_HEADER

#define ETHERNET_AUN_UDPPORT	0x8000
#define ETHERNET_SAUN_UDPPORT	0x8443

namespace ethernet {
	void	ipv4_Listener(void);
	int	transmitFrame(char *address, unsigned short port, econet::Frame *frame, int tx_length);
#ifdef ECONET_WITHOPENSSL
	void	ipv4_dtls_Listener (void);
	int	transmit_dtlsFrame(char *address, unsigned short port, econet::Frame *frame, int tx_length);
#endif
#ifdef ECONET_WITHIPV6
	void	ipv6_Listener(void);
#ifdef ECONET_WITHOPENSSL
	void	ipv6_dtls_Listener(void);
#endif
#endif
}
#endif
