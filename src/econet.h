/* econet.h
 * All Econet send and receive functions
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_ECONET_HEADER
#define ECONET_ECONET_HEADER

#include <cstddef>					// size_t
#include <cstdint>					// uint8_t

#define ECONET_MACHINETYPE		0x0314		// Econet machine type is RaspberryPi (3.14)
#define ECONET_MAX_FRAMESIZE		32768		// Maximum size of an Econet frame is 32768 bytes (todo: need to check what the maximum allowed framesize is according to Acorn specifications)
#define ECONET_MAX_SESSIONS		256		// Max 256 sessions for now; must rewrite the session handler someday to dynamically allocate sessions
#define ECONET_SESSION_TIMEOUT		60		// Each session will timeout after 60 seconds
#define ECONET_SESSION_TIMEOUT_NOTIFY	1		// Notify (send a frame to) stations when a session times out
#define ECONET_SERVERTYPE		"FILESTOR"	// Servertype when responding to &B0 FindServer

#define ECONET_FRAME_INVALID		0x01		// Is set when framesize < 4
#define ECONET_FRAME_BROADCAST		0x02		// Is set when frame is a broadcast
#define ECONET_FRAME_TOLOCAL		0x04		// Is set when frame is addressed to our local network
#define ECONET_FRAME_TOME		0x08		// Is set when frame is addressed to our station and network
#define ECONET_FRAME_SCOUT		0x10		// Is set when framesize = 6
#define ECONET_FRAME_ACK		0x20		// Is set when framesize = 4
#define ECONET_FRAME_DATA		0x40		// Is set when framesize > 4

#define ECONET_PORT_FILESERVER		0x99
#define ECONET_CONTROL_OSCLI		0x80
#define ECONET_CONTROL_PEEK		0x81
#define ECONET_CONTROL_POKE		0x82
#define ECONET_CONTROL_JSR		0x83
#define ECONET_CONTROL_USERPROC		0x84
#define ECONET_CONTROL_OSPROC		0x85
#define ECONET_CONTROL_HALT		0x86
#define ECONET_CONTROL_CONTINUE		0x87
#define ECONET_CONTROL_MACHINETYPE	0x88
#define ECONET_CONTROL_REGISTERS	0x89

#define ECONET_CONTROL_IP		0x81
#define ECONET_CONTROL_IPBCAST_REPLY	0x8E
#define ECONET_CONTROL_IPBCAST_REQUEST	0x8F
#define ECONET_CONTROL_ARP_REQUEST	0xA1
#define ECONET_CONTROL_ARP_REPLY	0xA2
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x
#define ECONET_CONTROL_		0x

#define ECONET_PRINTER_READY	0x00
#define ECONET_PRINTER_BUSY	0x01
#define ECONET_PRINTER_JAMMED	0x02

const uint8_t ECONET_BROADCAST_NEWBRIDGE[]	= {0xff, 0xff, 0x00, 0x00, 0x80, 0x9c, 0x00};
const uint8_t ECONET_BROADCAST_WHATNET[]	= {0xff, 0xff, 0x00, 0x00, 0x82, 0x9c, 'B', 'R', 'I', 'D', 'G', 'E', 0x9c, 0x00};

namespace econet {

	/* Network and station byte */
	typedef struct {
		uint8_t network;
		uint8_t station;
	} Station;

	/* Econet packet structure */
	typedef struct {
		union {
			uint8_t rawdata[ECONET_MAX_FRAMESIZE];
			union {
				struct {
					uint8_t dst_network;
					uint8_t dst_station;
					uint8_t src_network;
					uint8_t src_station;
					uint8_t data[ECONET_MAX_FRAMESIZE - 4];
				} econet;
				struct {
					uint8_t type;			/* Header: Transaction type */
					uint8_t port;			/* Header: Econet port */
					uint8_t control;		/* Header: Econet control byte */
					uint8_t retry;			/* Header: Retry */
					uint32_t sequence;		/* Header: 32-bit sequence number; little-endian encoded */
					union {
						uint8_t data[ECONET_MAX_FRAMESIZE - 8];
						struct {
							uint8_t replyport;		/* Data: Reply port */
							uint8_t function;		/* Data: Function */
							uint8_t urd;			/* Data: URD handle */
							uint8_t csd;			/* Data: CSD handle */
							uint8_t lib;			/* Lib handle */
						};
					};
				} aun;
			};

		};
		uint8_t flags;
		uint8_t control;
		uint8_t port;
	} Frame;

	/* To keep track of sessions with Econet clients */
	typedef struct {
		uint32_t sequence;
		uint64_t timeout;		// When this session will timeout (in Unix time)
		uint8_t network;
		uint8_t station;
		uint8_t port;
	} Session;

	typedef	int	(*ProtoHandlers)(const econet::Frame *, size_t, econet::Frame *, size_t);

	extern bool	netmon;
	extern Station	known_networks[256];
	extern Session	sessions[ECONET_MAX_SESSIONS];
	extern ProtoHandlers protohandlers[256];

	void	pollNetworkReceive(void);
	void	transmitFrame(econet::Frame *frame, unsigned int size);
	bool	validateFrame(econet::Frame *frame, int size);
	void	processFrame(econet::Frame *frame, int size);
	void	sendBridgeAnnounce(void);
	void	sendWhatNetBroadcast(void);
	bool	startSession(uint32_t sequence, unsigned char network, unsigned char station, unsigned char port);
	bool	hasSession(uint32_t sequence, unsigned char network, unsigned char station, unsigned char port);
	bool	endSession(uint32_t sequence, unsigned char network, unsigned char station, unsigned char port);
	int	port00handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length);
	int	port90handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length);
	int	port91handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length);
	int	port99handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length);
	int	port9Fhandler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length);
	int	portB0handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length);
	int	portD0handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length);
	int	portD1handler(const econet::Frame *rx_data, size_t rx_length, econet::Frame *tx_data, size_t tx_length);
	int	returnError(uint8_t *tx_data, size_t tx_length, uint32_t errorNumber);
}

#endif

