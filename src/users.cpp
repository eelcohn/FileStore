/* users.cpp
 * All User related functions
 *
 * (c) Eelco Huininga 2017-2018
 */

#include <cstdio>			// Included for EOF, printf(), FILE*, fopen(), fgets(), feof() and fclose()
#include <cstring>			// Included for memcpy(), strlen()

#include "users.h"			// 
#include "stations.h"			// Included for stations::stations[][]
#include "main.h"			// Included for main.h

using namespace std;



namespace users {
//	User *user;
	User user[256];
	int totalUsers = 0;

	/* Load !Users file */
	bool loadUsers(void) {
		int result;
		FILE *fp_usersfile;
		char a[256], b[256], c[256];

		printf("- Loading %s: ", USERSFILE);

		fp_usersfile = fopen(USERSFILE, "r");
		if (fp_usersfile != NULL) {
			while ((result = fscanf(fp_usersfile, "%s %s %s", a, b, c)) != EOF) {
				if ((result == 3) && (a[0] != '#')) {
					users::totalUsers++;
				}
			}
			fclose(fp_usersfile);
			printf("    %i users loaded.\n", users::totalUsers);
		} else {
			return(false);
		}
		return(true);
	}

	/* Log a user into the Econet FileStore
	   Returns: true	Login successfull
		    false	Login failed                      */
	bool login(char *username, char *pwhash, unsigned char network, unsigned char station) {
		int i;

		for (i = 0; i < totalUsers; i++) {
			if ((strcmp(user[i].username, username) == 0) && (strcmp(user[i].pwhash, pwhash) == 0)) {
				stations::stations[network - 1][station - 1].user = i;

				user[i].csd = '$';
				user[i].psd = '$';

//				if (exist("$.LIB"))
//					user[i].cld = '$';	// Set to handle for $.LIB
//				else
					user[i].cld = '$';	// Set to handle for $

				return(true);
			}
		}
		return(false);
	}


	/* Log a user off the Econet FileStore
	   Returns: true	Logout successfull
		    false	Logout failed                      */
	bool logout(char *username, unsigned char network, unsigned char station) {
		int i;

		for (i = 0; i < totalUsers; i++) {
			if (strcmp(user[i].username, username) == 0) {
				stations::stations[network - 1][station - 1].user = -1;
				return(true);
			}
		}
		return(false);
	}
}
