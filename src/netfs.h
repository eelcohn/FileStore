/* netfs.h
 * Handler for all NetFS commands
 *
 * (c) Eelco Huininga 2017-2018
 */

#ifndef ECONET_NETFS_HEADER
#define ECONET_NETFS_HEADER

namespace commands {
	int access(char **args);
	int cat(char **args);
	int cdir(char **args);
	int del(char **args);
	int i_am(char **args);
	int clockspeed(char **args);
}
#endif

