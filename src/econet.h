/* econet.h
 * All Econet send and receive functions
 *
 * (c) Eelco Huininga 2017-2018
 */

#ifndef ECONET_ECONET_HEADER
#define ECONET_ECONET_HEADER

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

namespace econet {
	typedef struct {
		unsigned char network;
		unsigned char station;
	} Station;
	typedef struct {
		unsigned char status;
		unsigned char dst_network, dst_station, src_network, src_station, control, port; // Must make this a union with data[] someday
		unsigned char data[ECONET_MAX_FRAMESIZE];
	} Frame;
	typedef struct {
		unsigned long timeout;		// When this session will timeout (in Unix time)
		unsigned char network;
		unsigned char station;
		unsigned char port;
	} Session;				// To keep track of sessions with Econet clients
	extern bool	netmon;
	extern Station known_networks[256];
	extern Session sessions[ECONET_MAX_SESSIONS];

	void	pollNetworkReceive(void);
	void	transmitFrame(econet::Frame *frame, int size);
	bool	validateFrame(econet::Frame *frame, int size);
	void	frameHandler(econet::Frame *frame, int size);
	bool	startSession(unsigned char network, unsigned char station, unsigned char port);
	bool	hasSession(unsigned char network, unsigned char station, unsigned char port);
	bool	endSession(unsigned char network, unsigned char station, unsigned char port);
	void	netmonPrintFrame(const char *interface, bool tx, econet::Frame *frame, int size);
}

#endif

