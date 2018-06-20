/* client.cpp
 * Client application for testing AUN (Econet over IP) based connections
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstring>
#include "client.h"
#include "dtls.h"

#define IP_PORT "127.0.0.1:33859"

bool	help = false;
bool	dtls = false;
bool	ipv4 = false;
bool	ipv6 = false;
int	host = -1;
char	*buffer;
size_t	numRead;

int main(int argc, char** argv) {
	int i, len;
	FILE *fp;

	/* Analyse command line parameters */
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0) {
			printf("%s", helpstring);
			exit(0);
		}

		if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--host") == 0)) {
			if (++i != argc)
				host = i;
			else {
				printf("--host: no parameter specified\n");
				exit(1);
			}
		}
		
		if ((strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--file") == 0)) {
			if (++i != argc)
				// Read the contents of the file (up to 4KB) into a buffer
				if ((fp = fopen(argv[i], "rb")) != NULL) {
					buffer = (char *)malloc(4096);
					numRead = fread(buffer, 1, 4096, fp);
				} else {
					printf("--file: file not found\n");
					exit(-1);
				}
			else {
				printf("--file: no parameter specified\n");
				exit(1);
			}
		}

		if ((strcmp(argv[i], "-x") == 0) || (strcmp(argv[i], "--hex") == 0)) {
			if (++i != argc) {
				buffer = (char *)malloc(4096);
				len = strlen(argv[i]);	
				for (size_t count = 0; count < (len / 2); count++) {
					sscanf(argv[i] + 2*count, "%02x", &buffer[count]);
				}
			} else {
				printf("--hex: no parameter specified\n");
				exit(1);
			}
		}

		if ((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--string") == 0)) {
			if (++i != argc) {
				buffer = argv[i];
				numRead = strlen(argv[i]);
			} else {
				printf("--string: no parameter specified\n");
				exit(1);
			}
		}

		if ((strcmp(argv[i], "-4") == 0) || (strcmp(argv[i], "--ipv4") == 0))
			ipv4 = true;

		if ((strcmp(argv[i], "-6")) || (strcmp(argv[i], "--ipv6")))
			ipv6 = true;

		if ((strcmp(argv[i], "-d")) || (strcmp(argv[i], "--dtls")))
			dtls = true;
	}

	if (dtls)
		dtls_connect();
	else
		udp_connect(argv[host], 32768);
}

void dtls_connect(void) {
	DTLSParams client;

	/* Initialize whatever OpenSSL state is necessary to execute the DTLS protocol. */
	dtls_Begin();

	/* Initialize the DTLS context from the keystore and then create the server SSL state */
	if (dtls_InitContextFromKeystore(&client, "client") < 0) {
		exit(EXIT_FAILURE);
	}
	if (dtls_InitClient(&client, IP_PORT) < 0) {
		exit(EXIT_FAILURE);
	}

	// Attempt to connect to the server and complete the handshake.
	int result = SSL_connect(client.ssl);
	if (result != 1) {
		perror("Unable to connect to the DTLS server.\n");
		exit(EXIT_FAILURE);
	}

	// Write the buffer to the server
	unsigned int written = SSL_write(client.ssl, buffer, numRead);
	if (written != numRead) {
		perror("Failed to write the entire buffer.\n");
		exit(EXIT_FAILURE);
	}

	int read = -1;
	do {
		// Read the output from the server. If it's not empty, print it.
		read = SSL_read(client.ssl, buffer, sizeof(buffer));
		if (read > 0) {
			printf("IN[%d]: ", read);
			for (int i = 0; i < read; i++) {
				printf("%c", buffer[i]);
			}
			printf("\n");
		}
	} while (read < 0);

	// Teardown the link and context state.
	dtls_Shutdown(&client);
}

void udp_connect(char *address, unsigned short port) {
	
}
