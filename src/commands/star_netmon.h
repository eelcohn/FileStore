/* netmon.h
 * *NETMON command handler
 *
 * (c) Eelco Huininga 2017-2018
 */

namespace commands {
	int netmon(char **args);
	void netmonPrintFrame(const char *interface, bool tx, econet::Frame *frame, int size);
}
bool kbhit(void);
