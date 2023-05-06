DISTFILES =	Makefile imsg-buffer.c imsg.c imsg.h

all:
	false

dist:
	mkdir -p ${DESTDIR}/
	${INSTALL} -m 0644 ${DISTFILES} ${DESTDIR}/

.PHONY: all dist
include ../../../config.mk
