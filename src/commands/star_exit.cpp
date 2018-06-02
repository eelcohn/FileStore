/* bye.cpp
 * *BYE command handler
 *
 * (c) Eelco Huininga 2017-2018
 */

using namespace std;

extern bool bye;



namespace commands {
	int exit(char **args) {
		bye = true;
		return(0);
	}
}


