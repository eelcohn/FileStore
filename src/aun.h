/* aun.h
 * All Ethernet send and receive functions
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_AUN_HEADER
#define ECONET_AUN_HEADER

#include "main.h"		// ./configure #define's
#include "econet.h"		// econet::Frame
#if (FILESTORE_WITHOPENSSL == 1)
#include <openssl/ssl.h>	/* SSL* */
#endif

enum {AUN_BROADCAST = 0x01, AUN_UNICAST, AUN_ACK, AUN_NAK, AUN_IMMEDIATE, AUN_IMMEDIATE_REPLY};

namespace aun {
	int	transmitFrame(econet::Frame *frame, unsigned int tx_length);
	int	ipv4_aun_Listener(void);
	int	ipv4_aun_Transmit(const char *address, unsigned short port, econet::Frame *frame, size_t tx_length);
#if (FILESTORE_WITHOPENSSL == 1)
	int	ipv4_dtls_Listener(void);
	int	ipv4_dtls_Transmit(const char *address, unsigned short port, econet::Frame *frame, size_t tx_length);
#endif
#if (FILESTORE_WITHIPV6 == 1)
	int	ipv6_aun_Listener(void);
	int	ipv6_aun_Transmit(const char *address, unsigned short port, econet::Frame *frame, size_t tx_length);
#if (FILESTORE_WITHOPENSSL == 1)
	int	ipv6_dtls_Listener(void);
	int	ipv6_dtls_Transmit(const char *address, unsigned short port, econet::Frame *frame, size_t tx_length);
#endif
#endif
	int	prepareAckPackage(econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length);
	int	rxHandler(econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length, bool *sendAck);
	bool	validateFrame(econet::Frame *data, size_t length);
}
#endif
