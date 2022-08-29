MDIR =		${HOME}/Mail/gameoftrees
OUTDIR =	/var/www/marc

.PHONY: all assets images dirs gzip clean scaleimgs

all: assets
	@env MDIR="${MDIR}" OUTDIR="${OUTDIR}" ./gotmarc

assets: dirs images ${OUTDIR}/style.css

images: ${OUTDIR}/got@2x.png ${OUTDIR}/got.png ${OUTDIR}/got-tiny@2x.png \
	${OUTDIR}/got-tiny.png

${OUTDIR}/got@2x.png: images/got.orig.png
	cp $? $@
${OUTDIR}/got.png: images/got.png
	cp $? $@
${OUTDIR}/got-tiny@2x.png: images/got-tiny@2x.png
	cp $? $@
${OUTDIR}/got-tiny.png: images/got-tiny.png
	cp $? $@
${OUTDIR}/style.css: style.css
	cp $? $@

dirs:
	@mkdir -p ${OUTDIR}/mail/
	@mkdir -p ${OUTDIR}/parts/
	@mkdir -p ${OUTDIR}/text/
	@mkdir -p ${OUTDIR}/thread/

gzip:
	gzip -fkr ${OUTDIR}

clean:
	rm -rf ${OUTDIR}

# -- maintainer targets --

scaleimgs: images/got.orig.png
	convert images/got.orig.png -resize 200x200 images/got.png
	convert images/got.orig.png -resize 128x128 images/got-tiny@2x.png
	convert images/got.orig.png -resize 64x64 images/got-tiny.png
