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
	void	pollAUNNetworkReceive(void);
	int	transmitFrame(econet::Frame *frame, int tx_length);
	void	pollSecureAUNNetworkReceive(void);
	int	transmitSecureAUNFrame(econet::Frame *frame, int tx_length);
void dtls_SAUNListener (void);
}

#endif

