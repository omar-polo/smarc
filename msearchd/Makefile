PROG =		msearchd
SRCS =		msearchd.c fcgi.c server.c
MAN =		msearchd.8

#DEBUG =	-O0 -g -DDEBUG

CDIAGFLAGS =	-Wall -Wuninitialized -Wshadow -Wunused
CDIAGFLAGS +=	-Wmissing-prototypes -Wstrict-prototypes
CDIAGFLAGS +=	-Werror

CPPFLAGS +=	-I/usr/local/include

LIBSQLITE3 =	/usr/local/lib/libsqlite3.a

LDADD = -levent -lsqlite3
DPADD = ${LIBEVENT} ${LIBSQLITE3}
LDFLAGS = -L/usr/local/lib

PREFIX ?=	/usr/local
SBINDIR ?=	${PREFIX}/sbin
MANDIR ?=	${PREFIX}/man/man
WWWDIR ?=	/var/www

realinstall:
	install -d -o root -g daemon -m 0755 ${DESTDIR}${WWWDIR}/msearchd
	install -m 0755 ${PROG} ${DESTDIR}${SBINDIR}

.include <bsd.prog.mk>
