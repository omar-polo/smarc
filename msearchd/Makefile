include config.mk
include ../config.mk

PROG =		msearchd
SRCS =		msearchd.c fcgi.c server.c
MAN =		msearchd.8

OBJS =		${SRCS:.c=.o} ${COMPATS:.c=.o}

# -- public targets --

all: ${PROG}

.PHONY: all tags clean distclean install uninstall dist

tags:
	ctags ${SRCS}

clean:
	rm -f *.[do] compat/*.[do] test/*.[do]

distclean: clean
	rm -f config.h config.mk

install:
	mkdir -p ${DESTDIR}${MANDIR}/man8
	${INSTALL_MAN} msearchd.8 ${DESTDIR}${MANDIR}/man8
	mkdir -p ${DESTDIR}${SBINDIR}
	${INSTALL_PROGRAM} ${PROG} ${DESTDIR}${SBINDIR}
	mkdir -p ${DESTDIR}${SYSCONFDIR}/gotmarc
	${INSTALL_DATA} schema.sql ${DESTDIR}${SYSCONFDIR}/gotmarc

uninstall:
	rm -f ${DESTDIR}${MANDIR}/man8/msearchd.8
	rm -f ${DESTDIR}${SBINDIR}/${PROG}
	rm -f ${DESTDIR}${SYSCONFDIR}/gotmarc/schema.sql

# -- internal build targets --

${PROG}: ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LDFLAGS}

.c.o:
	${CC} -c $< -o $@ -DSYSCONFDIR="\"${REAL_SYSCONFDIR}\"" ${CFLAGS}

# -- maintainer targets --

DISTFILES =	Makefile configure ${SRCS} msearchd.h msearchd.8 schema.sql

dist:
	mkdir -p ${DESTDIR}/
	${INSTALL} -m 0644 ${DISTFILES} ${DESTDIR}/
	chmod 0755 ${DESTDIR}/configure
	${MAKE} -C compat DESTDIR=${DESTDIR}/compat dist
	${MAKE} -C tests  DESTDIR=${DESTDIR}/tests  dist

# -- dependencies --

-include fcgi.d
-include msearchd.d
-include server.d
