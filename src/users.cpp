/* users.cpp
 * All User related functions
 *
 * (c) Eelco Huininga 2017-2019
 */

#include <cstdio>			// Included for EOF, printf(), FILE*, fopen(), fgets(), feof() and fclose()
#include <cstdlib>			// srand(), rand()
#include <cstring>			// Included for memcpy(), strlen()
#include <openssl/evp.h>		// EVP_MD
#include <openssl/kdf.h>		// EVP_PKEY_CTX_set1_pbe_pass, EVP_PKEY_CTX_set1_scrypt_salt, EVP_PKEY_CTX_set_scrypt_N, EVP_PKEY_CTX_set_scrypt_r, EVP_PKEY_CTX_set_scrypt_p

#include "users.h"			// 
#include "settings.h"			// settings::defaultflags
#include "stations.h"			// Included for stations::stations[][]
#include "main.h"			// Included for main.h

using namespace std;



namespace users {
//	User *user;
	User users[MAX_USERS] = {"", "", "", {false, false, false, false, false, false}, -1, 0, 0, 0, 0, 0};
	Session sessions[MAX_SESSIONS];
	unsigned int totalUsers = 0;
	unsigned int totalSessions = 0;

	/*********************************************************************/
	/* Load users from the !Users file                                   */
	/* Returns: true	Users successfully loaded                    */
	/*	    false	Loading users failed                         */
	/*********************************************************************/
	int loadUsers(void) {
		char	flags[MAX_USER_FLAGS];
	 	char	asciihash[FILESTORE_USERS_HASH_LENGTH];
			char	buffer[256];
		FILE	*fp_usersfile;
		size_t	i;

		printf("- Loading %s: ", USERSFILE);

		fp_usersfile = fopen(USERSFILE, "r");
		if (fp_usersfile != NULL) {
			while (!feof(fp_usersfile)) {
				if (fgets(buffer, sizeof(buffer), fp_usersfile) != NULL) {
					if (buffer[0] != '#') {
						/* TODO: there's no size checking when fscanf-ing the values into users[]. If a string in the !Users file is larger than the size of the variables in users[], a buffer overvlow will happen */
						if (sscanf(buffer, "%10s %64s %128s %8s", users[totalUsers].username, users[totalUsers].salt, asciihash, flags) == 4) {
							if (atobin(asciihash, sizeof(asciihash), users[totalUsers].pwhash, sizeof(users[totalUsers].pwhash)) != 0) {
								for (i = 0; i < strlen(flags); i++) {
									switch (flags[i]) {
										case 'P' :
											users[totalUsers].flags.p = true;
											break;

										case 'S' :
											users[totalUsers].flags.s = true;
											break;

										case 'N' :
											users[totalUsers].flags.n = true;
											break;

										case 'E' :
											users[totalUsers].flags.e = true;
											break;

										case 'L' :
											users[totalUsers].flags.l = true;
											break;

										case 'R' :
											users[totalUsers].flags.r = true;
											break;

										default :
											printf("Warning: unknown flag \"%c\" in user file\n", flags[i]);
											break;
									}
								}
								users::totalUsers++;
							} else {
								fprintf(stderr, "Warning: invalid password hash %s\n", asciihash);
							}
						}
					}
				}
			}
			fclose(fp_usersfile);
			printf("    %i users loaded.\n", users::totalUsers);
		} else {
			return(0x000000D6);
		}
		return(0);
	}

	/*********************************************************************/
	/* Log a user into the Econet FileStore                              */
	/* Returns: true	Logout successfull                           */
	/*	    false	Logout failed                                */
	/*********************************************************************/
	int login(unsigned int user_id, const char *password, unsigned char network, unsigned char station) {
		unsigned char	hash[SHA512_DIGEST_LENGTH];

		if (user_id < totalUsers) {
			if ((users::getSession(user_id, network, station)) == -1) {
				if (PKCS5_PBKDF2_HMAC(password, strlen(password), users::users[user_id].salt, sizeof(users::users[user_id].salt), PASSWORD_HASHING_ITERATIONS, (const EVP_MD*) EVP_sha512(), sizeof(hash), hash) == 1) {
	 				if ((memcmp(users::users[user_id].pwhash, hash, sizeof(hash))) == 0) {
						users::newSession(user_id, network, station);

						users[user_id].csd = '$';
						users[user_id].psd = '$';

//						if (exist("$.LIB"))
//							users[user_id].cld = '$';	// Set to handle for $.LIB
//						else
							users[user_id].cld = '$';	// Set to handle for $

						return(0);
					} else {
						return(0x000000BB);
					}
				} else {
					fprintf(stderr, "Error generating PBKDF2 hash\n");
					return(0x12345678);
				}
			} else {
				return (0x00001234);
			}
		} else {
			return(0x000000BC);
		}
		return(0x12345678);
	}

	/*********************************************************************/
	/* Log a user off the Econet FileStore                               */
	/* Returns: true	Logout successfull                           */
	/*	    false	Logout failed                                */
	/*********************************************************************/
	int logout(unsigned int user_id, unsigned char network, unsigned char station) {
		int session_id;

		if (user_id < totalUsers) {
			session_id = users::getSession(user_id, network, station);
			if (session_id != -1) {
				return (users::delSession(session_id));
			}
		}

		return(0x000000AE);
	}

	/*********************************************************************/
	/* Creates a new user                                                */
	/* Returns: true	New user created successfull                 */
	/*	    false	Creating new user failed                     */
	/*********************************************************************/
	int newUser(const char *username, const char *password) {
		unsigned char	salt[FILESTORE_USERS_SALT_LENGTH] = {""};
		const char	saltcharset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
		unsigned char	binaryhash[SHA512_DIGEST_LENGTH];
	 	char		asciihash[FILESTORE_USERS_HASH_LENGTH];
		FILE		*fp_usersfile;
		size_t		i;
		time_t		t;

		if (users::getUserID(username) == -1) {
			/* Generate a random salt for the new user */
			srand((unsigned) time(&t));
			for (i = 0; i < (sizeof(salt) - 1); i++) {
				salt[i] = saltcharset[rand() % (sizeof(saltcharset) - 1)];
			}
			salt[i] = '\0';

			/* Generate the PBKDF2 hash for the password/salt combination */
//			if (PKCS5_SCRYPT_HMAC(args[2], strlen(args[2]), (const unsigned char *)salt, sizeof(salt), iterations, 1024, 8, 16, sizeof(binaryhash), binaryhash) == 0) {
			if (PKCS5_PBKDF2_HMAC(password, strlen(password), salt, sizeof(salt), PASSWORD_HASHING_ITERATIONS, (const EVP_MD*) EVP_sha512(), sizeof(binaryhash), binaryhash) == 0) {
				fprintf(stderr, "Error generating PBKDF2 hash");
				return(0x12345678);
			}
			/* Add the new user to our internal users::users[] array */
			strlcpy(users[totalUsers].username, username, sizeof(users[totalUsers].username));
			strlcpy((char *)users[totalUsers].salt, (char *)salt, sizeof(users[totalUsers].salt));
			memcpy((char *)users[totalUsers].pwhash, (char *)binaryhash, sizeof(users[totalUsers].pwhash));
			users[totalUsers].flags.p = false;
			users[totalUsers].flags.s = false;
			users[totalUsers].flags.n = false;
			users[totalUsers].flags.e = false;
			users[totalUsers].flags.l = false;
			users[totalUsers].flags.r = false;
			totalUsers++;

			/* Add user to the !Users file */
			bintoa(binaryhash, sizeof(binaryhash), asciihash, sizeof(asciihash));

			fp_usersfile = fopen(USERSFILE, "a");
			if (fp_usersfile != NULL) {
				if (fprintf(fp_usersfile, "%s	%s	%s	%s\n", username, salt, asciihash, settings::defaultflags) < 0) {
					fprintf(stderr, "Error writing new user to !Users file.\n");
					fclose(fp_usersfile);
					return(0x00000025);
				}
			} else {
				fprintf(stderr, "Error opening !Users file.\n");
				return(0x00000025);
			}

			return (true);
		} else {
			fprintf(stderr, "User %s already exists\n", username);
			return (0x12345678);
		}
	}

	/*********************************************************************/
	/* Change the password for an user_id                                */
	/* Returns: true	New user created successfull                 */
	/*	    false	Creating new user failed                     */
	/*********************************************************************/
	int changePassword(unsigned int user_id, const char *curpw, const char *newpw) {
		unsigned char	oldbinhash[SHA512_DIGEST_LENGTH];
		unsigned char	newbinhash[SHA512_DIGEST_LENGTH];
	 	char		asciihash[FILESTORE_USERS_HASH_LENGTH];
		char		flags[MAX_USER_FLAGS];
		FILE		*fp_usersfile;
		size_t		i;

		if (user_id < totalUsers) {
			if (users::users[user_id].flags.p == true) {
				/* Generate the PBKDF2 hash for the password/salt combination */
//				if (PKCS5_SCRYPT_HMAC(args[2], strlen(args[2]), (const unsigned char *)salt, sizeof(salt), iterations, 1024, 8, 16, sizeof(binaryhash), binaryhash) == 0) {
				if (PKCS5_PBKDF2_HMAC(curpw, strlen(curpw), users::users[user_id].salt, sizeof(users::users[user_id].salt), PASSWORD_HASHING_ITERATIONS, (const EVP_MD*) EVP_sha512(), sizeof(oldbinhash), oldbinhash) == 0) {
					fprintf(stderr, "Error generating PBKDF2 hash for old password");
					return(0x12345678);
				}

				/* Check if the old password is correct */
				if ((memcmp(users::users[user_id].pwhash, oldbinhash, sizeof(oldbinhash))) == 0) {
					if (PKCS5_PBKDF2_HMAC(newpw, strlen(newpw), users::users[user_id].salt, sizeof(users::users[user_id].salt), PASSWORD_HASHING_ITERATIONS, (const EVP_MD*) EVP_sha512(), sizeof(newbinhash), newbinhash) == 0) {
						fprintf(stderr, "Error generating PBKDF2 hash for new password");
						return(0x12345678);
					}

					/* Change the password hash in the internal users::users[] array */
					strlcpy((char *)users[totalUsers].pwhash, (char *)newbinhash, sizeof(users[totalUsers].pwhash));

					fp_usersfile = fopen(USERSFILE ".new", "a");
					if (fp_usersfile != NULL) {
						for (i = 0; i < users::totalUsers; i++) {
							bintoa(users::users[i].pwhash, sizeof(users::users[i].pwhash), asciihash, sizeof(asciihash));
							users::getUserFlags(i, flags);
							if (fprintf(fp_usersfile, "%s	%s	%s	%s\n", users::users[i].username, users::users[i].salt, asciihash, flags) < 0) {
								fclose(fp_usersfile);
								return(0x00000025);
							}
						}
						fclose(fp_usersfile);
						return(0);
					} else {
						fprintf(stderr, "Error opening !Users file.\n");
						return(0x00000025);
					}
				} else {
					return(0x000000B9);
				}
			} else {
				return (0x000000C3);
			}
		} else {
			return(0x000000BC);
		}
	}

	/*********************************************************************/
	/* Find the user_id for a given username                             */
	/* Returns: user_id	The user_id for the given username           */
	/*	    -1		No user was found for given username         */
	/*********************************************************************/
	int getUserID(const char *username) {
		unsigned int i;

		for (i = 0; i < totalUsers; i++) {
			if ((strcmp(users[i].username, username)) == 0) {
				return (i);
			}
		}

		return (-1);
	}

	int getSession(unsigned int user_id, unsigned char network, unsigned char station) {
		unsigned int i;

		for (i = 0; i < totalSessions; i++) {
			if ((users::sessions[i].network == network) && (users::sessions[i].station == station) && (users::sessions[i].user_id == user_id)) {
				return (i);
			}
		}

		return (-1);
	}

	int newSession(unsigned int user_id, unsigned char network, unsigned char station) {
		unsigned int i;

		/* Scan for the first free session_id which is available */
		for (i = 0; i < MAX_SESSIONS; i++) {
			if (users::sessions[i].login_time == 0) {
				users::sessions[i].network = network;
				users::sessions[i].station = station;
				users::sessions[i].user_id = user_id;
				users::sessions[i].login_time = time(NULL);
				totalSessions++;
				return (i);
			}
		}

		return (1);
	}
	int delSession(unsigned int session_id) {
		if (users::sessions[session_id].login_time != 0) {
			users::sessions[session_id].network = 0;
			users::sessions[session_id].station = 0;
			users::sessions[session_id].user_id = 0;
			users::sessions[session_id].login_time = 0;
			totalSessions--;
			return (0);
		}

		return (1);
	}

	int getUserFlags(unsigned int user_id, char *flags) {
		int i;

		i = 0;
		if (users::users[user_id].flags.p)
			flags[i++] = 'P';
		if (users::users[user_id].flags.s)
			flags[i++] = 'S';
		if (users::users[user_id].flags.n)
			flags[i++] = 'N';
		if (users::users[user_id].flags.e)
			flags[i++] = 'E';
		if (users::users[user_id].flags.l)
			flags[i++] = 'L';
		if (users::users[user_id].flags.r)
			flags[i++] = 'R';
		flags[i] = '\0';

		return(0);
	}

	int getBootOption(uint8_t bootoption, char *bootstr) {
		switch (bootoption) {
			case 0 :
				strlcpy(bootstr, "Off", sizeof(bootstr));
				break;

			case 1 :
				strlcpy(bootstr, "Load", sizeof(bootstr));
				break;

			case 2 :
				strlcpy(bootstr, "Run", sizeof(bootstr));
				break;

			case 3 :
				strlcpy(bootstr, "Exec", sizeof(bootstr));
				break;

			default :
				strlcpy(bootstr, "????", sizeof(bootstr));
				break;
		}

		return(0);
	}
}

int PKCS5_SCRYPT_HMAC(const char *pass, int passlen, const unsigned char *salt, int saltlen, const unsigned int N, const unsigned int r, const unsigned int p, size_t outlen, unsigned char *out) {
	EVP_PKEY_CTX *pctx;

	pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_SCRYPT, NULL);

	if (EVP_PKEY_derive_init(pctx) <= 0) {
		fprintf(stderr, "EVP_PKEY_derive_init");
		return -1;
	}

	if (EVP_PKEY_CTX_set1_pbe_pass(pctx, pass, passlen) <= 0) {
		fprintf(stderr, "EVP_PKEY_CTX_set1_pbe_pass");
		return -1;
	}

	if (EVP_PKEY_CTX_set1_scrypt_salt(pctx, salt, saltlen) <= 0) {
		fprintf(stderr, "EVP_PKEY_CTX_set1_scrypt_salt");
		return -1;
	}

	if (EVP_PKEY_CTX_set_scrypt_N(pctx, N) <= 0) {
		fprintf(stderr, "EVP_PKEY_CTX_set_scrypt_N");
		return -1;
	}

	if (EVP_PKEY_CTX_set_scrypt_r(pctx, r) <= 0) {
		fprintf(stderr, "EVP_PKEY_CTX_set_scrypt_r");
		return -1;
	}

	if (EVP_PKEY_CTX_set_scrypt_p(pctx, p) <= 0) {
		fprintf(stderr, "EVP_PKEY_CTX_set_scrypt_p");
		return -1;
	}

	if (EVP_PKEY_derive(pctx, out, &outlen) <= 0) {
		fprintf(stderr, "EVP_PKEY_derive");
		return -1;
	}

	return 0;
}

/* Convert raw binary data to ASCII hex string */
size_t bintoa(const unsigned char *binary, const size_t binary_len, char *ascii, const size_t ascii_len) {
	size_t i;

	i = 0;

	/* Make sure that the size of *ascii is big enough for all the contents of *binary */
	if (ascii_len < (binary_len * 2))
		return(0);

	for(i = 0; i < binary_len; i++) {
		sprintf(ascii + (i * 2), "%02x", binary[i]);
	}
	ascii[ascii_len - 1] = 0;

	return(binary_len);
}

/* Convert ASCII hex string to raw binary data */
size_t atobin(const char *ascii, const size_t ascii_len, unsigned char *binary, const size_t binary_len) {
	size_t i;
 
	/* Make sure that the size of *binary is big enough for all the contents of *ascii */
	if (ascii_len < (binary_len * 2))
		return 0;

	/* Make sure that *ascii has an even number of hexadecimal characters */
	if (binary_len % 2 != 0)
		return 0;

	/* Validate that ascii contains a valid hex string */
	if (ascii[strspn(ascii, "0123456789abcdefABCDEF")] != 0)
		return 0;

	for(i = 0; i < binary_len; i++) {
		binary[i]  = ((ascii[(i * 2)    ] & 0xDF) < 'A') ? (((ascii[(i * 2)    ] & 0xDF) - 0x10) << 4) : (((ascii[(i * 2)    ] & 0xDF) - 0x37) << 4);
		binary[i] |= ((ascii[(i * 2) + 1] & 0xDF) < 'A') ?  ((ascii[(i * 2) + 1] & 0xDF) - 0x10)       :  ((ascii[(i * 2) + 1] & 0xDF) - 0x37)      ;
	}

	return ascii_len;
}

