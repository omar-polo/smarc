MDIR =		${HOME}/Maildir/op/GoT
OUTDIR =	www

.PHONY: all assets images dirs gzip clean

all: assets
	env MDIR="${MDIR}" OUTDIR="${OUTDIR}" ./gotmarc

assets: dirs images ${OUTDIR}/style.css

images: ${OUTDIR}/got@2x.png ${OUTDIR}/got.png ${OUTDIR}/got-tiny@2x.png \
	${OUTDIR}/got-tiny.png

${OUTDIR}/got@2x.png: got.png
	cp got.png ${OUTDIR}/got@2x.png
${OUTDIR}/got.png: got.png
	convert got.png -resize 200x200 ${OUTDIR}/got.png
${OUTDIR}/got-tiny@2x.png: got.png
	convert got.png -resize 128x128 ${OUTDIR}/got-tiny@2x.png
${OUTDIR}/got-tiny.png: got.png
	convert got.png -resize 64x64 ${OUTDIR}/got-tiny.png
${OUTDIR}/style.css: style.css
	cp style.css ${OUTDIR}

dirs:
	mkdir -p ${OUTDIR}/mail/
	mkdir -p ${OUTDIR}/mbox/
	mkdir -p ${OUTDIR}/parts/
	mkdir -p ${OUTDIR}/text/
	mkdir -p ${OUTDIR}/thread/

gzip:
	gzip -fkr ${OUTDIR}/

clean:
	rm -rf ${OUTDIR}
