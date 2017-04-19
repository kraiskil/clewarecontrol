// SVN: $Revision: 56 $
#include <errno.h>
#include <libintl.h>
#include <locale.h>
#include <stdarg.h>
#include <string.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define _ gettext

void error_exit(const bool ie, const char *format, ...)
{
	int e = errno;

	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	if (ie)
		fprintf(stderr, _("\nThe system indicated: %s (%d)\n"), strerror(e), e);

	exit(EXIT_FAILURE);
}
