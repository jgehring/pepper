#!/bin/sh

PEPPER="pepper"
REPOS_ROOT="$HOME/data/repos"

if [ -n "$1" ]; then
	PEPPER="$1"
	export PEPPER_REPORTS="`dirname $1`/../reports"
fi

mkdir -p "img"
rm -f *.png
rm -f *.svg

export GNUTERM=pngcairo

# Let's go
echo "loc_hg.png"
$PEPPER -q loc --tags="^%d.%d$" -s800 "$REPOS_ROOT/hg" > loc_hg.png
echo "authors_hg.png"
$PEPPER -q authors --tags="^%d.%d$" -s800 "$REPOS_ROOT/hg" > authors_hg.png
echo "activity_hg.png"
$PEPPER -q activity --split=directories -n4 --tags="^%d%.%d$" -s800 "$REPOS_ROOT/hg" > activity_hg.png

echo "activity_linux.png"
$PEPPER -q activity --split=directories -n6 --tags="^v[%d%.]+$" -s800 "$REPOS_ROOT/linux" > activity_linux.png
echo "punchcard_linux.png"
$PEPPER -q punchcard -s1024 --author="Linus Torvalds" "$REPOS_ROOT/linux" > punchcard_linux.png

echo "files_git.png"
$PEPPER -q activity --split=files -n4 --tags="^v%d%.%d%.0$" -s800 "$REPOS_ROOT/git" > files_git.png
echo "times_git.png"
$PEPPER -q times -s1024 "$REPOS_ROOT/git" > times_git.png

echo "volume_mediawiki.png"
$PEPPER -q volume -c --slots=a,4y,2y,y -s800x400 "svn://rudy/mediawiki" > volume_mediawiki.png
echo "files_mediawiki.png"
$PEPPER -q filetypes --pie -s800 "svn://rudy/mediawiki/trunk" > files_mediawiki.png

echo "commits_firefox"
$PEPPER -q commit_counts  -p18m -s1024 "$REPOS_ROOT/mozilla-central" > commits_firefox.png
echo "authors_firefox"
$PEPPER -q activity --split=authors -s800 "$REPOS_ROOT/mozilla-central" > authors_firefox.png 


# Create thumbnails
echo -n "Generating thumbnails"
for i in *.png; do
	convert -thumbnail 350 -strip -quality 95 -unsharp 0x1 -gravity center -extent 350x265 $i PNG8:"`basename $i .png`-thumb.png"
	echo -n "."
done
echo

# Cleanup temporary files
rm -f *.out
