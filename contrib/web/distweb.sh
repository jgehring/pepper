#!/bin/bash
#
#	distweb <srcdir> <output-dir>
#

SRCDIR="$1"
if [ -z "${SRCDIR}" ]; then
	echo "No srcdir given"
	exit 1
fi
OUTDIR="$2"
if [ -z "${OUTDIR}" ]; then
	echo "No output directory given"
	exit 1
fi

echo "Cleaning output directory... "
rm -rf "${OUTDIR}"
mkdir -p "${OUTDIR}/examples"

echo "Copying static files... "
cp "${SRCDIR}/web/index.html" "${OUTDIR}"
cp "${SRCDIR}/web/stylesheet.css" "${OUTDIR}"
cp "${SRCDIR}/web/background.png" "${OUTDIR}"
cp "${SRCDIR}/README" "${OUTDIR}"
cp "${SRCDIR}/ChangeLog" "${OUTDIR}"
cp "${SRCDIR}/CREDITS" "${OUTDIR}"

echo "Updating and copying luadoc... "
(cd "${SRCDIR}"; make -s luadoc)
cp -r "${SRCDIR}/luadoc" "${OUTDIR}/"

echo "Substituting variables... "
DATE=$(date +"%B %d, %Y")
sed -i "s/\$DATE/${DATE}/" "${OUTDIR}/index.html"

echo "Generating example images (might take a long time)"
"${SRCDIR}/contrib/web/genexamples.sh" "${SRCDIR}" "${OUTDIR}/examples"
