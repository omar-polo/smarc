ENV =		MBLAZE=.mblaze OUTDIR='${OUTDIR}'
MBLAZE_PAGER =	cat
MDIR =		${HOME}/Maildir/op/GoT
OUTDIR =	www

.PHONY: all gzip dirs assets clean

all: .mblaze dirs assets
	mlist '${MDIR}' | mthread -r | \
		${ENV} mscan -f '%R %I %i %16D <%64f> %128S' | \
		${ENV} ./mexp | ${ENV} ./mkindex > ${OUTDIR}/index.html

gzip:
	gzip -fkr ${OUTDIR}/

dirs:
	mkdir -p ${OUTDIR}/mbox/
	mkdir -p ${OUTDIR}/parts/
	mkdir -p ${OUTDIR}/text/
	mkdir -p ${OUTDIR}/thread/

assets: dirs got.png style.css
	cp got.png ${OUTDIR}/got@2x.png
	convert got.png -resize 200x200 ${OUTDIR}/got.png
	convert got.png -resize 128x128 ${OUTDIR}/got-tiny@2x.png
	convert got.png -resize 64x64 ${OUTDIR}/got-tiny.png
	cp style.css ${OUTDIR}

${OUTDIR}:
	mkdir -p '${OUTDIR}'

.mblaze:
	mkdir -p .mblaze
	touch .mblaze/seq

clean:
	rm -rf ${OUTDIR}
