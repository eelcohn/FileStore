/* users.h
 * All User related functions
 *
 * (c) Eelco Huininga 2017-2018
 */

#ifndef ECONET_USERS_HEADER
#define ECONET_USERS_HEADER

#define MAX_USERNAME	10
#define MAX_PRIVCHARS	1

#include <openssl/sha.h>		// Included for SHA256_DIGEST_LENGTH


typedef struct {
	char		username[MAX_USERNAME];
	char		pwhash[SHA256_DIGEST_LENGTH];
	char		priv[MAX_PRIVCHARS];
	int		currentDisc;			// Currently selected disc in the discs[] array
	unsigned char	bootoption;			// Boot option (as set by *OPT 4,x
	unsigned char	csd;				// Handle for the Currently Selected Directory
	unsigned char	psd;				// Handle for the Previously Selected Directory
	unsigned char	cld;				// Handle for the Currently Selected Library
} User;

namespace users {
	extern User user[256];

	void Users(char *u, char *h, char *p);
	bool loadUsers(void);
	bool login(char *username, char *pwhash, unsigned char network, unsigned char station);
	bool logout(char *username, unsigned char network, unsigned char station);
};
#endif

