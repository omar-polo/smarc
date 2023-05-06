DISTFILES =	Makefile filter seq

all:
	false

dist:
	mkdir -p ${DESTDIR}/
	${INSTALL} -m 0644 ${DISTFILES} ${DESTDIR}/

install:
	mkdir -p ${DESTDIR}${SHAREDIR}/gotmarc/mblaze
	${INSTALL_DATA} filter seq ${DESTDIR}${SHAREDIR}/gotmarc/mblaze

uninstall:
	rm -f ${DESTDIR}${SHAREDIR}/gotmarc/mblaze/filter
	rm -f ${DESTDIR}${SHAREDIR}/gotmarc/mblaze/seq

.PHONY: all dist install uninstall
include ../config.mk
