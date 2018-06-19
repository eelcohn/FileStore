#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>			// AF_INET, AF_INET6, SOCK_DGRAM, SOL_SOCKET, SO_REUSEADDR, SO_RCVTIMEO, setsockopt(), bind()
#include <arpa/inet.h>			// inet_aton(), inet_pton()
#include <netinet/in.h>			// Included for IPPROTO_UDP

#include "dtls.h"

/* Create a new socket */
static int _createSocket(int family, char *address, unsigned short port) {
	int sock;
	int reuseconn;
	struct timeval timeout;

	/* Create a new socket */
	if ((sock = socket(family, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("_createSocket: Unable to create socket");
		exit(EXIT_FAILURE);
	}

	/* Set socket to allow multiple connections */
	reuseconn = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseconn, sizeof(reuseconn)) == -1) {
		perror("_createSocket: setsockopt(SO_REUSEADDR)");
	}

	/* Set timeout on socket to prevent recvfrom from blocking execution */
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		perror("_createSocket: Error setting timeout on socket");
	}

	switch (family) {
		case AF_INET :
			struct sockaddr_in addr;

			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			if (address == NULL)
				addr.sin_addr.s_addr = htonl(INADDR_ANY);
			else {
				if (inet_pton(AF_INET, address, (void *)&addr.sin_addr.s_addr) != 1) {
					perror("_createSocket: Invalid IPv4 address");
					exit(EXIT_FAILURE);
				}
			}

			if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
				perror("_createSocket: IPv4 Unable to bind");
				exit(EXIT_FAILURE);
			}

			break;

		case AF_INET6 :
			struct sockaddr_in6 addr6;

			addr6.sin6_family = AF_INET6;
			addr6.sin6_port = htons(port);
			addr6.sin6_scope_id = 0;
			if (address == NULL)
				addr6.sin6_addr = in6addr_any;
			else {
				if (inet_pton(AF_INET6, address, (void *)&addr6.sin6_addr.s6_addr) != 1) {
					perror("_createSocket: Invalid IPv6 address");
					exit(EXIT_FAILURE);
				}
			}

			if (bind(sock, (struct sockaddr*) &addr6, sizeof(addr6)) < 0) {
				perror("_createSocket: IPv6 Unable to bind");
				exit(EXIT_FAILURE);
			}

			break;

		default :
			perror("_createSocket: unknown family");
			exit(EXIT_FAILURE);
	}

//	if (listen(sock, 1) < 0) {
//		perror("Unable to listen");
//		exit(EXIT_FAILURE);
//	}

	return sock;
}

void dtls_Begin() {
	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();
}

void dtls_End() {
	ERR_remove_thread_state(NULL);
	ENGINE_cleanup();
	CONF_modules_unload(1);
	ERR_free_strings();
	EVP_cleanup();
	sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
	CRYPTO_cleanup_all_ex_data();
}

static int _ssl_verify_peer(int ok, X509_STORE_CTX* ctx) {
	return 1;
}

int dtls_InitContextFromKeystore(DTLSParams* params, const char* keyname) {
	int result = 0;

	// Create a new context using DTLS
	params->ctx = SSL_CTX_new(DTLSv1_2_method());
	if (params->ctx == NULL) {
		printf("Error: cannot create SSL_CTX.\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}

	// Set our supported ciphers
	result = SSL_CTX_set_cipher_list(params->ctx, "ALL:!ADH:!LOW:!EXP:!MD5:!SSL2:!SSL3:@STRENGTH");
	if (result != 1) {
		printf("Error: cannot set the cipher list.\n");
		ERR_print_errors_fp(stderr);
		return -2;
	}

	// The client doesn't have to send it's certificate
	SSL_CTX_set_verify(params->ctx, SSL_VERIFY_PEER, _ssl_verify_peer);

	// Load key and certificate
	char certfile[1024];
	char keyfile[1024];
	sprintf(certfile, "./%s-cert.pem", keyname);
	sprintf(keyfile, "./%s-key.pem", keyname);

	// Load the certificate file; contains also the public key
	result = SSL_CTX_use_certificate_file(params->ctx, certfile, SSL_FILETYPE_PEM);
	if (result != 1) {
		printf("Error: cannot load certificate file.\n");
		ERR_print_errors_fp(stderr);
		return -4;
	}

	// Load private key
	result = SSL_CTX_use_PrivateKey_file(params->ctx, keyfile, SSL_FILETYPE_PEM);
	if (result != 1) {
		printf("Error: cannot load private key file.\n");
		ERR_print_errors_fp(stderr);
		return -5;
	}

	// Check if the private key is valid
	result = SSL_CTX_check_private_key(params->ctx);
	if (result != 1) {
		printf("Error: checking the private key failed. \n");
		ERR_print_errors_fp(stderr);
		return -6;
	}

	return 0;
}

int dtls_InitClient(DTLSParams* params, const char *address) {
	params->bio = BIO_new_ssl_connect(params->ctx);
	if (params->bio == NULL) {
		fprintf(stderr, "error connecting to server\n");
		return -1;
	}

	BIO_set_conn_hostname(params->bio, address);
	BIO_get_ssl(params->bio, &(params->ssl));
	if (params->ssl == NULL) {
		fprintf(stderr, "error, exit\n");
		return -1;
	}

	SSL_set_connect_state(params->ssl);
	SSL_set_mode(params->ssl, SSL_MODE_AUTO_RETRY);

	return 0;
}

int dtls_InitServer(DTLSParams* params) {
	params->bio = BIO_new_ssl_connect(params->ctx);
	if (params->bio == NULL) {
		fprintf(stderr, "error connecting with BIOs\n");
		return -1;
	}

	BIO_get_ssl(params->bio, &(params->ssl));
	if (params->ssl == NULL) {
		fprintf(stderr, "error, exit\n");
		return -1;
	}

	SSL_set_accept_state(params->ssl);

	return 0;
}

void dtls_Shutdown(DTLSParams* params) {
	if (params != NULL) {
		if (params->ctx != NULL) {
			SSL_CTX_free(params->ctx);
			params->ctx = NULL;
		}

		if (params->ssl != NULL) {
			SSL_free(params->ssl);
			params->ssl = NULL;
		}
	}
}
