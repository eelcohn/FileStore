/* client.cpp
 * Client application for testing AUN (Econet over IP) based connections
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstring>
#include <sys/socket.h>			// AF_INET, AF_INET6
#include <unistd.h>			// close()
#include <arpa/inet.h>			// inet_aton(), inet_pton()
#include "client.h"
#include "../dtls.h"

#define IP_PORT "127.0.0.1:33589"

struct options {
bool	help = false;
bool	dtls = false;
bool	ipv4 = false;
bool	ipv6 = false;
int	host = -1;
int	port = -1;
} options;

char	*buffer;
size_t	numRead;



int main(int argc, char** argv) {
	int i;
	size_t len;
	FILE *fp;

	/* Analyse command line parameters */
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0) {
			printf("%s", helpstring);
			exit(0);
		}

		if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--host") == 0)) {
			if (++i != argc)
				options.host = i;
			else {
				printf("--host: no parameter specified\n");
				exit(1);
			}
		}
		
		if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "--port") == 0)) {
			if (++i != argc)
				options.port = i;
			else {
				printf("--port: no parameter specified\n");
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
			options.ipv4 = true;

		if ((strcmp(argv[i], "-6")) || (strcmp(argv[i], "--ipv6")))
			options.ipv6 = true;

		if ((strcmp(argv[i], "-d")) || (strcmp(argv[i], "--dtls")))
			options.dtls = true;
	}

	if (options.dtls)
		dtls_connect(argv[options.host], atoi(argv[options.port]));
	else
		udp_connect(argv[options.host], atoi(argv[options.port]));
}

int dtls_connect(const char *address, const unsigned short port) {
	DTLSParams server;

//	BIO *conn;
//	DH *dh;
	FILE *paramfile;
//	SSL *ssl;
//	SSL_CTX *ctx;
	char buf[200] = {0};
	int err, sock;

	SSL_library_init();
	SSL_load_error_strings();
	paramfile = fopen("conf/keys/dh2048.pem", "r");
	if (paramfile == NULL) {
		fprintf(stderr, "Error opening the DH file: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	server.dh = PEM_read_DHparams(paramfile, NULL, NULL, NULL);
	fclose(paramfile);

	if (server.dh == NULL) {
		fprintf(stderr, "Error reading the DH file\n");
		return EXIT_FAILURE;
	}

	server.socket = makesock(33859);

	server.bio = BIO_new_dgram(server.socket, BIO_NOCLOSE);
	if (server.bio == NULL) {
		fprintf(stderr, "error creating bio\n");
		return EXIT_FAILURE;
	}

	server.ctx = SSL_CTX_new(DTLS_server_method());
	if (server.ctx == NULL) {
		ERR_print_errors_fp(stderr);
		return EXIT_FAILURE;
	}

	err = SSL_CTX_set_cipher_list(server.ctx, "HIGH:MEDIUM:aNULL");
	if (err == 0) {
		ERR_print_errors_fp(stderr);
		return EXIT_FAILURE;
	}

	err = SSL_CTX_set_tmp_dh(server.ctx, server.dh);
	if (err == 0) {
		ERR_print_errors_fp(stderr);
		return EXIT_FAILURE;
	}
printf("SSL_CTX_set_tmp_dh done\n");
	server.ssl = SSL_new(server.ctx);
	if (server.ssl == NULL) {
		return EXIT_FAILURE;
	}
printf("SSL_new done\n");
	SSL_set_bio(server.ssl, server.bio, server.bio);
printf("SSL_set_bio done\n");
	SSL_set_accept_state(server.ssl);
printf("SSL_set_accept_state done\n");
	err = SSL_read(server.ssl, &buf, 199);
printf("Read done\n");
	if (err <= 0) {
		err = SSL_get_error(server.ssl, err);
		fprintf(stderr, "SSL_read: error %d\n", err);
		ERR_print_errors_fp(stderr);
		return EXIT_FAILURE;
	}
	fprintf (stderr, "%s", buf);

	const char c[] = "Hello yourself!\n";
	err = SSL_write(server.ssl, c, strlen(c));
printf("Write done\n");
	if (err <= 0) {
		err = SSL_get_error(server.ssl, err);
		fprintf(stderr, "SSL write: error %d\n", err);
		ERR_print_errors_fp(stderr);
		if (err == SSL_ERROR_SYSCALL)
			fprintf(stderr, "errno: %s\n", strerror(errno));
	}
	SSL_free(server.ssl);	/* free conn too */
	SSL_CTX_free(server.ctx);
	DH_free(server.dh);
	close(server.socket);
return 0;

}

void udp_connect(const char *address, const unsigned short port) {
	int socket;

	if (options.ipv6)
		socket = dtls::createSocket(AF_INET6, address, port);
	else
		socket = dtls::createSocket(AF_INET, address, port);
	
}

static int makesock(int port) {
	int sock = 0;
	struct sockaddr_in addr;

	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		fprintf (stderr, "socket: %s\n", strerror (errno ));
		exit (EXIT_FAILURE);
	}
#ifdef SERVER
	/∗ inutile pour le client ∗/
	if (bind(sock, (struct sockaddr∗) &addr, sizeof addr)) {
		fprintf (stderr , "bind: %s\n", strerror (errno ));
		exit (EXIT_FAILURE);
	}
#endif
	return sock;
}

