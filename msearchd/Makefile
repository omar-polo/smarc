PROG =		msearchd
SRCS =		msearchd.c fcgi.c server.c

DEBUG =		-O0 -g -DDEBUG

CDIAGFLAGS =	-Wall -Wuninitialized -Wshadow -Wunused
CDIAGFLAGS +=	-Wmissing-prototypes -Wstrict-prototypes
CDIAGFLAGS +=	-Werror

CPPFLAGS +=	-I${.CURDIR}/template
CPPFLAGS +=	-I/usr/local/include

LIBSQLITE3 =	/usr/local/lib/libsqlite3.a

LDADD = -levent -lsqlite3
DPADD = ${LIBEVENT} ${LIBSQLITE3}
LDFLAGS = -L/usr/local/lib

.include <bsd.prog.mk>
