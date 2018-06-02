/* newuser.cpp
 * *NEWUSER command handler
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>			// Included for printf() and sprintf()
#include <cstring>			// Included for strlen()
#include "openssl/sha.h"		// Included for SHA256_CTX, SHA256_Init, SHA256_Update, SHA256_Final and SHA256_DIGEST_LENGTH

using namespace std;



namespace commands {
	int newuser(char **args) {
	 	char		outputBuffer[65];
		unsigned char	hash[SHA256_DIGEST_LENGTH];
		int		i;
		SHA256_CTX	sha256;

		SHA256_Init(&sha256);
		SHA256_Update(&sha256, args[2], strlen(args[2]));
		SHA256_Final(hash, &sha256);

		i = 0;
		for(i = 0; i < SHA256_DIGEST_LENGTH; i++) {
			sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
		}
		outputBuffer[64] = 0;
		printf("Added new user %s with password hash %s\n", args[1], outputBuffer);

		return(0);
	}
}


