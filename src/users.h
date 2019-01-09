/* users.h
 * All User related functions
 *
 * (c) Eelco Huininga 2017-2019
 */

#ifndef ECONET_USERS_HEADER
#define ECONET_USERS_HEADER

#define MAX_USERNAME	10
#define MAX_USER_FLAGS	8
#define MAX_USERS	256
#define MAX_SESSIONS	256
#define PASSWORD_HASHING_ITERATIONS	1000
#define MAX_PASSWORD_LENGTH 256
#define FILESTORE_USERS_HASH_LENGTH (SHA512_DIGEST_LENGTH * 2) + 1
#define FILESTORE_USERS_SALT_LENGTH (SHA512_DIGEST_LENGTH + 1)

#include <ctime>			// time_t
#include <openssl/sha.h>		// Included for SHA256_DIGEST_LENGTH


typedef struct {
	char		username[MAX_USERNAME];
	uint8_t		salt[FILESTORE_USERS_SALT_LENGTH];
	uint8_t		pwhash[SHA512_DIGEST_LENGTH];
	struct {			/* Attributes (encoded in bit 7 of Name) */
		bool	p;		/* Password unlocked */
		bool	s;		/* System privileged */
		bool	n;		/* No short SAVEs */
		bool	e;		/* Permanent *ENABLE */
		bool	l;		/* No library */
		bool	r;		/* Run only user */
	} flags;
	int		currentDisc;			// Currently selected disc in the discs[] array
	uint8_t		bootoption;			// Boot option (as set by *OPT 4,x
	uint8_t		csd;				// Handle for the Currently Selected Directory
	uint8_t		psd;				// Handle for the Previously Selected Directory
	uint8_t		cld;				// Handle for the Currently Selected Library
	uint8_t		enable_counter;			// Flag to check if an user did *ENABLE
} User;

typedef struct {
	uint8_t		network;
	uint8_t		station;
	uint32_t	user_id;
	time_t		login_time;
} Session;

namespace users {
	extern User	users[MAX_USERS];
	extern Session	sessions[MAX_SESSIONS];
	extern uint32_t	totalUsers;
	extern uint32_t	totalSessions;

	void Users(char *u, char *h, char *p);
	int loadUsers(void);
	int login(unsigned int user_id, const char *pwhash, unsigned char network, unsigned char station);
	int logout(unsigned int user_id, unsigned char network, unsigned char station);
	int newUser(const char *username, const char *password);
	int changePassword(unsigned int user_id, const char *curpw, const char *newpw);
	int getUserID(const char *username);
	int getSession(unsigned int user_id, unsigned char network, unsigned char station);
	int newSession(unsigned int user_id, unsigned char network, unsigned char station);
	int delSession(unsigned int session_id);
	int getUserFlags(unsigned int user_id, char *flags);
	int getBootOption(uint8_t bootoption, char *bootstr);
}

int PKCS5_SCRYPT_HMAC(const char *pass, int passlen, const unsigned char *salt, int saltlen, const unsigned int N, const unsigned int r, const unsigned int p, size_t outlen, unsigned char *out);
size_t bintoa(const unsigned char *binary, const size_t binary_len, char *ascii, const size_t ascii_len);
size_t atobin(const char *ascii, const size_t ascii_len, unsigned char *binary, const size_t binary_len);
#endif

