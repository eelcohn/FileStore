#include <cstdio>
//#include "../commands/configure.h"

int print(char *buffer)
{
//	FILE *printer = popen("/usr/bin/lpr -P PDF", "w");
	FILE *printer = popen("/usr/bin/lpr", "w");

	if (printer == NULL)
		return (-1);

	fprintf(printer, "%s", buffer);
	pclose(printer);

	return(0);
}
