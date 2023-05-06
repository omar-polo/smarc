include config.mk

# -- build-related variables --

VERSION =	0.1
DISTNAME =	gotmarc-${VERSION}

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
	${INSTALL_PROGRAM} gmimport ${DESTDIR}${BINDIR}
	sed	-e "/^libexec=/s@=.*@=${LIBEXEC}/gotmarc@" \
		-e "/^mblaze=/s@=.*@=${SHAREDIR}/gotmarc/mblaze@" \
		-e "/^tmpldir=/s@=.*@=${REAL_SYSCONFDIR}/gotmarc@" \
		gotmarc > ${DESTDIR}${BINDIR}/gotmarc
	chmod 0755 ${DESTDIR}${BINDIR}/gotmarc
	mkdir -p ${DESTDIR}${LIBEXEC}/gotmarc
	${INSTALL_PROGRAM} filter-ignore ${DESTDIR}${LIBEXEC}/gotmarc
	${INSTALL_PROGRAM} mexp ${DESTDIR}${LIBEXEC}/gotmarc
	${INSTALL_PROGRAM} mkindex ${DESTDIR}${LIBEXEC}/gotmarc
	${INSTALL_PROGRAM} pe ${DESTDIR}${LIBEXEC}/gotmarc
	mkdir -p ${DESTDIR}${SYSCONFDIR}/gotmarc
	${INSTALL_DATA} style.css ${DESTDIR}${SYSCONFDIR}/gotmarc/
	mkdir -p ${DESTDIR}${PERL_LIB}
	${INSTALL_DATA} GotMArc.pm ${DESTDIR}${PERL_LIB}
	mkdir -p ${DESTDIR}${MANDIR}/man1
	${INSTALL_MAN} gmimport.1 ${DESTDIR}${MANDIR}/man1/
	${INSTALL_MAN} gotmarc.1 ${DESTDIR}${MANDIR}/man1/
	mkdir -p ${DESTDIR}${MANDIR}/man7
	${INSTALL_MAN} gotmarc.7 ${DESTDIR}${MANDIR}/man7/
	${MAKE} -C .mblaze   install
	${MAKE} -C msearchd  install
	${MAKE} -C templates install

uninstall:
	rm -f ${DESTDIR}${BINDIR}/gmimport
	rm -f ${DESTDIR}${BINDIR}/gotmarc
	rm -f ${DESTDIR}${LIBEXEC}/gotmarc/filter-ignore
	rm -f ${DESTDIR}${LIBEXEC}/gotmarc/mexp
	rm -f ${DESTDIR}${LIBEXEC}/gotmarc/mkindex
	rm -f ${DESTDIR}${LIBEXEC}/gotmarc/pe
	rm -f ${DESTDIR}${SYSCONFDIR}/gotmarc/style.css
	rm -f ${DESTDIR}${PERL_LIB}/GotMArc.pm
	rm -f ${DESTDIR}${MANDIR}/man1/gmimport.1
	rm -f ${DESTDIR}${MANDIR}/man1/gotmarc.1
	rm -f ${DESTDIR}${MANDIR}/man7/gotmarc.7
	${MAKE} -C .mblaze   uninstall
	${MAKE} -C msearchd  uninstall
	${MAKE} -C templates uninstall

# -- maintainer targets --

PRIVKEY =	missing-PRIVKEY
PUBKEY =	missing-PUBKEY
DISTFILES =	GotMArc.pm Makefile README TODO configure \
		filter-ignore gmimport gmimport.1 gotmarc gotmarc.1 \
		gotmarc.7 mexp mkindex pe style.css

release:
	sed -i -e '/^RELEASE=/s/no/yes/' configure
	${MAKE} ${DISTNAME}.sha256.sig
	sed -i -e '/^RELEASE=/s/yes/no/' configure

dist: ${DISTNAME}.sha256

${DISTNAME}.sha256.sig: ${DISTNAME}.sha256
	signify -S -e -m ${DISTNAME}.sha256 -s ${PRIVKEY}

${DISTNAME}.sha256: ${DISTNAME}.tar.gz
	sha256 ${DISTNAME}.tar.gz > $@

${DISTNAME}.tar.gz: ${DISTFILES}
	mkdir -p .dist/${DISTNAME}
	${INSTALL} -m 0644 ${DISTFILES} .dist/${DISTNAME}
	cd .dist/${DISTNAME} && chmod 0755 configure filter-ignore \
		gmimport gotmarc mexp mkindex pe
	${MAKE} -C .mblaze   DESTDIR=${PWD}/.dist/${DISTNAME}/.mblaze   dist
	${MAKE} -C templates DESTDIR=${PWD}/.dist/${DISTNAME}/templates dist
	${MAKE} -C msearchd  DESTDIR=${PWD}/.dist/${DISTNAME}/msearchd  dist
	cd .dist && tar czf ../$@ ${DISTNAME}
	rm -rf .dist

.PHONY: release ${DISTNAME}.tar.gz
