include config.mk

# -- build-related variables --

VERSION =	0.1
DISTNAME =	smarc-${VERSION}

# -- public targets --

all: msearchd
.PHONY: all msearchd tags clean distclean install uninstall

msearchd:
	${MAKE} -C msearchd

tags:
	${MAKE} -C msearchd tags

clean:
	${MAKE} -C msearchd clean

distclean: clean
	${MAKE} -C msearchd distclean
	rm -f config.log config.log.old config.mk

install:
	mkdir -p ${DESTDIR}${BINDIR}
	${INSTALL_PROGRAM} smingest ${DESTDIR}${BINDIR}
	sed	-e "/^libexec=/s@=.*@=${LIBEXEC}/smarc@" \
		-e "/^mblaze=/s@=.*@=${SHAREDIR}/smarc/mblaze@" \
		-e "/^tmpldir=/s@=.*@=${REALSYSCONFDIR}/smarc@" \
		smarc > ${DESTDIR}${BINDIR}/smarc
	chmod 0755 ${DESTDIR}${BINDIR}/smarc
	mkdir -p ${DESTDIR}${LIBEXEC}/smarc
	${INSTALL_PROGRAM} filter-ignore ${DESTDIR}${LIBEXEC}/smarc
	${INSTALL_PROGRAM} mexp ${DESTDIR}${LIBEXEC}/smarc
	${INSTALL_PROGRAM} mkindex ${DESTDIR}${LIBEXEC}/smarc
	${INSTALL_PROGRAM} pe ${DESTDIR}${LIBEXEC}/smarc
	mkdir -p ${DESTDIR}${SYSCONFDIR}/smarc
	${INSTALL_DATA} style.css ${DESTDIR}${SYSCONFDIR}/smarc/
	mkdir -p ${DESTDIR}${PERL_LIB}
	${INSTALL_DATA} SMArc.pm ${DESTDIR}${PERL_LIB}
	mkdir -p ${DESTDIR}${MANDIR}/man1
	${INSTALL_MAN} smingest.1 ${DESTDIR}${MANDIR}/man1/
	${INSTALL_MAN} smarc.1 ${DESTDIR}${MANDIR}/man1/
	mkdir -p ${DESTDIR}${MANDIR}/man7
	${INSTALL_MAN} smarc.7 ${DESTDIR}${MANDIR}/man7/
	${MAKE} -C .mblaze   install
	${MAKE} -C msearchd  install
	${MAKE} -C templates install

uninstall:
	rm -f ${DESTDIR}${BINDIR}/smingest
	rm -f ${DESTDIR}${BINDIR}/smarc
	rm -f ${DESTDIR}${LIBEXEC}/smarc/filter-ignore
	rm -f ${DESTDIR}${LIBEXEC}/smarc/mexp
	rm -f ${DESTDIR}${LIBEXEC}/smarc/mkindex
	rm -f ${DESTDIR}${LIBEXEC}/smarc/pe
	rm -f ${DESTDIR}${SYSCONFDIR}/smarc/style.css
	rm -f ${DESTDIR}${PERL_LIB}/SMArc.pm
	rm -f ${DESTDIR}${MANDIR}/man1/smingest.1
	rm -f ${DESTDIR}${MANDIR}/man1/smarc.1
	rm -f ${DESTDIR}${MANDIR}/man7/smarc.7
	${MAKE} -C .mblaze   uninstall
	${MAKE} -C msearchd  uninstall
	${MAKE} -C templates uninstall

# -- maintainer targets --

PRIVKEY =	missing-PRIVKEY
PUBKEY =	missing-PUBKEY
DISTFILES =	CHANGES Makefile README SMArc.pm TODO configure \
		filter-ignore mexp mkindex pe smarc smarc.1 smarc.7 \
		smingest smingest.1 style.css

MANOPTS = man='%N.%S.html;https://man.openbsd.org/%N.%S',style=mandoc.css,toc
MANFLAGS =	-Thtml -O${MANOPTS}

man:
	touch msearchd.8
	man ${MANFLAGS} -l smingest.1 > smingest.1.html
	man ${MANFLAGS} -l smarc.1 > smarc.1.html
	man ${MANFLAGS} -l smarc.7 > smarc.7.html
	man ${MANFLAGS} -l msearchd/msearchd.8  > msearchd.8.html
	rm msearchd.8

release:
	sed -i -e '/^RELEASE=/s/no/yes/' configure
	${MAKE} ${DISTNAME}.sha256.sig
	sed -i -e '/^RELEASE=/s/yes/no/' configure

verify:
	signify -C -p ${PUBKEY} -x ${DISTNAME}.sha256.sig

dist: ${DISTNAME}.sha256

${DISTNAME}.sha256.sig: ${DISTNAME}.sha256
	signify -S -e -m ${DISTNAME}.sha256 -s ${PRIVKEY}

${DISTNAME}.sha256: ${DISTNAME}.tar.gz
	sha256 ${DISTNAME}.tar.gz > $@

${DISTNAME}.tar.gz: ${DISTFILES}
	mkdir -p .dist/${DISTNAME}
	${INSTALL} -m 0644 ${DISTFILES} .dist/${DISTNAME}
	cd .dist/${DISTNAME} && chmod 0755 configure filter-ignore \
		smingest smarc mexp mkindex pe
	${MAKE} -C .mblaze   DESTDIR=${PWD}/.dist/${DISTNAME}/.mblaze   dist
	${MAKE} -C templates DESTDIR=${PWD}/.dist/${DISTNAME}/templates dist
	${MAKE} -C msearchd  DESTDIR=${PWD}/.dist/${DISTNAME}/msearchd  dist
	cd .dist && tar czf ../$@ ${DISTNAME}
	rm -rf .dist

.PHONY: man release verify dist ${DISTNAME}.tar.gz
