/* debug.h
 * Debug and logging options
 *
 * (c) Eelco Huininga 2017
 */

namespace debug {
	void networkLogStart(void);
	void networkLog(void);
	void networkLogStop(void);
#ifdef DEBUGBUILD
	int read(char **args);
	int write(char **args);
	int d(char **args);
	int rs(char **args);
	int rw(char **args);
	int cs(char **args);
	int rst(char **args);
	int phi(char **args);
#endif
}

