#!/usr/bin/env python
#
# Generates example reports for pepper's homepage
#

import sys, subprocess, os, glob

if len(sys.argv) < 3:
	print("Usage: %s <srcdir> <outdir>" % sys.argv[0])
	sys.exit(1)

srcdir = sys.argv[1]
outdir = sys.argv[2]

repos = {
	"pepper" : { "dir" : srcdir, "branch" : "master", "tags" : "" },
	"hg" : { "dir" : os.path.expanduser("~/data/repos/hg"), "branch" : "default", "tags" : "^[0-9].[0-9]$" },
	"git" : { "dir" : os.path.expanduser("~/data/repos/git"), "branch" : "master", "tags" : "^v[1-9].[4-9].[0-9]$" },
	"linux" : { "dir" : os.path.expanduser("~/data/repos/linux-2.6"), "branch" : "master", "tags" : "^v2.6.[0-9]*$" }
}
reports = {
	"loc" : { "file" : "loc" },
	"authors" : { "file" : "authors" },
	"directories" : { "file" : "directories" },
	"commits_per_month" : { "file" : "cpm" }
}

for repo in repos:
	print(">> Generating reports for " + repo)
	for report in reports:
		print(">>> " + report)
		subprocess.check_call([srcdir + "/src/pepper", "--quiet", srcdir + "/reports/" + report, "--branch=" + repos[repo]["branch"], "--tags=" + repos[repo]["tags"], "--output=" + reports[report]["file"] + ".svg", repos[repo]["dir"]])
		os.rename(reports[report]["file"] + ".svg", outdir + "/" + reports[report]["file"] + "-" + repo + ".svg")

print(">> Generating thumbnails")
for f in glob.glob(outdir + "/*.svg"):
	print(">>> " + f)
	subprocess.check_call(["rsvg", "--x-zoom=2.0", "--y-zoom=2.0", f, f.replace(".svg", ".png")])
	subprocess.check_call(["rsvg", "--x-zoom=0.7", "--y-zoom=0.7", f, f.replace(".svg", "-thumb.png")])
