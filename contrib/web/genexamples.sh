#!/bin/bash
#
#	genexamples <srcdir> <output-dir>
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

PEPPER="${SRCDIR}/src/pepper"
REPORTS="${SRCDIR}/reports"

echo -n "Generating SVGs for "
echo -n "pepper "
#"${PEPPER}" -q "${REPORTS}/loc" -tsvg -bmaster "${SRCDIR}"
#mv loc.svg "${OUTDIR}/loc-pepper.svg"
"${PEPPER}" -q "${REPORTS}/authors" -tsvg -bmaster "${SRCDIR}"
mv authors.svg "${OUTDIR}/authors-pepper.svg"
"${PEPPER}" -q "${REPORTS}/commits_per_month" -tsvg -bmaster "${SRCDIR}"
mv cpm.svg "${OUTDIR}/cpm-pepper.svg"

echo -n "mercurial "
#"${PEPPER}" -q "${REPORTS}/loc" -tsvg "${HOME}/data/repos/hg"
#mv loc.svg "${OUTDIR}/loc-hg.svg"
"${PEPPER}" -q "${REPORTS}/authors" -tsvg "${HOME}/data/repos/hg"
mv authors.svg "${OUTDIR}/authors-hg.svg"
"${PEPPER}" -q "${REPORTS}/commits_per_month" -tsvg "${HOME}/data/repos/hg"
mv cpm.svg "${OUTDIR}/cpm-hg.svg"

echo -n "git "
#"${PEPPER}" -q "${REPORTS}/loc" -tsvg "${HOME}/data/repos/git"
#mv loc.svg "${OUTDIR}/loc-git.svg"
"${PEPPER}" -q "${REPORTS}/authors" -tsvg "${HOME}/data/repos/git"
mv authors.svg "${OUTDIR}/authors-git.svg"
"${PEPPER}" -q "${REPORTS}/commits_per_month" -tsvg "${HOME}/data/repos/git"
mv cpm.svg "${OUTDIR}/cpm-git.svg"

echo -n "linux-2.6 "
#"${PEPPER}" -q "${REPORTS}/loc" -tsvg "${HOME}/data/repos/linux-2.6"
#mv loc.svg "${OUTDIR}/loc-linux.svg"
"${PEPPER}" -q "${REPORTS}/authors" -tsvg "${HOME}/data/repos/linux-2.6"
mv authors.svg "${OUTDIR}/authors-linux.svg"
"${PEPPER}" -q "${REPORTS}/commits_per_month" -tsvg "${HOME}/data/repos/linux-2.6"
mv cpm.svg "${OUTDIR}/cpm-linux.svg"

echo 
echo "Rendering PNG images... "
cd ${OUTDIR}
for i in *.svg; do
	rsvg --x-zoom=2.0 --y-zoom=2.0 ${i} "`basename ${i} .svg`.png"
	rsvg --x-zoom=0.7 --y-zoom=0.7 ${i} "`basename ${i} .svg`-thumb.png"
done
