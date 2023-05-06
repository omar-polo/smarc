#include "../config.h"

#ifndef __dead
# define __dead __attribute__((noreturn))
#endif

__dead void	err(int, const char *, ...)
		    __attribute__((__format__ (printf, 2, 3)));
__dead void	errx(int, const char *, ...)
		    __attribute__((__format__ (printf, 2, 3)));
void		warn(int, const char *, ...)
		    __attribute__((__format__ (printf, 1, 2)));
void		warnx(int, const char *, ...)
		    __attribute__((__format__ (printf, 1, 2)));
