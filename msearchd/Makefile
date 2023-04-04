PROG =		msearchd
SRCS =		msearchd.c fcgi.c server.c
MAN =		msearchd.8

#DEBUG =	-O0 -g -DDEBUG

CDIAGFLAGS =	-Wall -Wuninitialized -Wunused
CDIAGFLAGS +=	-Wmissing-prototypes -Wstrict-prototypes
CDIAGFLAGS +=	-Werror

WARNINGS =	Yes

CPPFLAGS +=	-I/usr/local/include

LIBSQLITE3 =	/usr/local/lib/libsqlite3.a

LDADD = -levent -lsqlite3
DPADD = ${LIBEVENT} ${LIBSQLITE3}
LDFLAGS = -L/usr/local/lib

PREFIX ?=	/usr/local
SBINDIR ?=	${PREFIX}/sbin
MANDIR ?=	${PREFIX}/man/man
WWWDIR ?=	/var/www
RCDIR ?=	/etc/rc.d/

realinstall:
	install -d -o root -g daemon -m 0755 ${DESTDIR}${WWWDIR}/msearchd
	install -m 0755 ${PROG} ${DESTDIR}${SBINDIR}
	install -m 0555 ${.CURDIR}/${PROG}.rc ${DESTDIR}${RCDIR}/${PROG}

.include <bsd.prog.mk>
