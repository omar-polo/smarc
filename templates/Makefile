DISTFILES =	Makefile foot.html head.html index-header.html \
		logo-small.html search-header.html search.html

all:
	false

dist:
	mkdir -p ${DESTDIR}/
	${INSTALL} -m 0644 ${DISTFILES} ${DESTDIR}/

install:
	mkdir -p ${DESTDIR}${SYSCONFDIR}/gotmarc/
	${INSTALL_DATA} ${DISTFILES} ${DESTDIR}${SYSCONFDIR}/gotmarc/

uninstall:
	rm -f ${DESTDIR}${SYSCONFDIR}/gotmarc/${DISTFILES}

.PHONY: all dist install uninstall

include ../config.mk
