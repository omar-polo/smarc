DISTFILES =	Makefile filter seq

all:
	false

dist:
	mkdir -p ${DESTDIR}/
	${INSTALL} -m 0644 ${DISTFILES} ${DESTDIR}/

install:
	mkdir -p ${DESTDIR}${SHAREDIR}/smarc/mblaze
	${INSTALL_DATA} filter seq ${DESTDIR}${SHAREDIR}/smarc/mblaze

uninstall:
	rm -f ${DESTDIR}${SHAREDIR}/smarc/mblaze/filter
	rm -f ${DESTDIR}${SHAREDIR}/smarc/mblaze/seq

.PHONY: all dist install uninstall
include ../config.mk
