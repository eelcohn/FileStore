/* netmon.cpp
 * *NETMON command handler
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>			// Included for NULL, printf(), FILE*, fopen(), fgets(), feof() and fclose()
#include <cstring>			// Included for strncpy()
#include "../econet.h"			// Included for econet::netmon and econet::Frame
#include "../main.h"			// Included for bye
#include "star_netmon.h"

extern std::atomic<bool>	bye;

using namespace std;



namespace commands {
	int netmon(char **args) {
		econet::netmon = true;
		printf("Netmon started\n");

		while (bye == false) {
			// The econet::pollNetworkReceive, econet::sendFrame, ethernet::pollNetworkReceive and ethernet::sendFrame will check if econet::netmon is set to true.
			// If it is, they will print the sent or received frame to stdin
			if (kbhit()) {
				getc(stdin);
				bye = true;
			}
		}

		econet::netmon = false;
		bye = false;
		printf("\n");
		printf(PROMPT);
		return(0);
	}

	/* Hex-dump a frame to stdout */
	void netmonPrintFrame(const char *interface, bool tx, econet::Frame *frame, int size) {
		int i, addr;
		char valid[8];

		if (tx)
			printf("Tx");
		else
			printf("Rx");

		if (frame->status | ECONET_FRAME_INVALID)
			strcpy(valid, "invalid");
		else
			strcpy(valid, "valid  ");

		printf("  %s  %s  dst=%02X:%02X  src=%02X:%02X  ctrl=%02X  port=%02X  size=%i bytes\n", interface, valid, frame->dst_network, frame->dst_station, frame->src_network, frame->src_station, frame->control, frame->port, size);

		addr = 0;
		while ((size - addr) > 0) {
			printf("%04X  ", addr);
			for (i = addr; i < ((size - addr) & 0xf); i++) {
				printf("%02X ", frame->data[i]);
			}
			printf(" ");
			for (i = addr; i < ((size - addr) & 0xf); i++) {
				if ((frame->data[i] > 31) && (frame->data[i] < 127))
					printf("%c", frame->data[i]);
				else
					printf(".");
			}
			printf("\n");
			addr += 16;
		}
	}
}

bool kbhit(void) {
	struct timeval tv;
	fd_set read_fd;

	/* Do not wait at all, not even a microsecond */
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	/* Must be done first to initialize read_fd */
	FD_ZERO(&read_fd);

	/* Makes select() ask if input is ready: 0 is the file descriptor for stdin */
	FD_SET(0,&read_fd);

	/* The first parameter is the number of the largest file descriptor to check + 1. */
	if(select(1, &read_fd,NULL, /*No writes*/NULL, /*No exceptions*/&tv) == -1)
		return(false); /* An error occured */

	/* read_fd now holds a bit map of files that are
	* readable. We test the entry for the standard
	* input (file 0). */

	if(FD_ISSET(0,&read_fd))
		/* Character pending on stdin */
		return(true);

	/* no characters were pending */
	return(false);
}
