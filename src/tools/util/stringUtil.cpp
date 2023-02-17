#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"

char *strtolower(char *s) {
	if (s == 0) {
		return 0;
	}
	for (int i = 0; s[i]; ++i) {
		s[i] = tolower(s[i]);
	}
	return s;
}
