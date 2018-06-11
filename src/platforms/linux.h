/* linux.h
 * Linux-specific functions
 *
 * (c) Eelco Huininga 2017-2018
 */

void	initSignals(void);
void	sigHandler(int sig);
int	print(char *buffer);

