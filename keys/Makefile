NEXTV =		04
DISTFILES =	Makefile smarc-01.pub smarc-02.pub smarc-03.pub smarc-04.pub

all:
	false

newkey:
	signify -G -p smarc-${NEXTV}.pub -s smarc-${NEXTV}.sec

dist:
	mkdir -p ${DESTDIR}
	${INSTALL} -m 0644 ${DISTFILES} ${DESTDIR}/

.PHONY: all newkey dist
include ../config.mk
