DISTFILES =	Makefile foot.html head.html index-header.html \
		logo-small.html search-header.html search-link.html \
		search.html

all:
	false

dist:
	mkdir -p ${DESTDIR}/
	${INSTALL} -m 0644 ${DISTFILES} ${DESTDIR}/

install:
	mkdir -p ${DESTDIR}${SYSCONFDIR}/smarc/
	${INSTALL_DATA} *.html ${DESTDIR}${SYSCONFDIR}/smarc/

uninstall:
	rm -f ${DESTDIR}${SYSCONFDIR}/smarc/${DISTFILES}

.PHONY: all dist install uninstall

include ../config.mk
