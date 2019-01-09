#include <cstring>

/* Capitalize a string */
char * strtoupper(char *s) {
	int l;

	l = strlen(s);
	for (int i = 0; i < l; i++)
		if (s[i] >= 'a' and s[i] <= 'z')
			s[i] = s[i] - 32;

	return s;
}

