#!/usr/bin/env ruby
#
# Generates example reports for pepper's homepage
#

if ARGV.length < 2
	puts("Usage: %s <srcdir> <outdir>" % $0)
	exit(1)
end

srcdir = ARGV[0]
outdir = ARGV[1]
home = ENV["HOME"]
ENV["PEPPER_REPORTS"] = srcdir + "/reports"

repos = {
	"pepper" => { :dir => srcdir, :branch => "master", :tags => "" },
	"hg" => { :dir => home + "/data/repos/hg", :branch => "default", :tags => "'^[0-9].[0-9]$'" },
	"git" => { :dir => home + "/data/repos/git", :branch => "master", :tags => "'^v[1-9].[4-9].[0-9]$'" },
	"linux" => { :dir => home + "/data/repos/linux-2.6", :branch => "master", :tags => "'^v2.6.[0-9]*$'" }
}
reports = {
	"authors" => { :file => "authors" },
	"loc" => { :file => "loc" },
	"activity" => { :file => "activity", :options => [ "--split=directories" ] },
	"commit_counts" => { :file => "commits" },
	"times" => { :file => "times" },
	"filetypes" => { :file => "filetypes" }
}

repos.keys.each do |repo|
	puts(">> Generating reports for #{repo}")
	reports.keys.each do |report|
		puts(">>> #{report}")

		cmd = "#{srcdir}/src/pepper --quiet #{report}"
		cmd = cmd + " --branch=#{repos[repo][:branch]} --tags=#{repos[repo][:tags]}"
		cmd = cmd + " --output=#{outdir}/#{reports[report][:file]}-#{repo}.svg"

		if reports[report][:options]
			reports[report][:options].each do |opt|
				cmd = cmd + " " + opt
			end
		end
		cmd = cmd + " " + repos[repo][:dir]

		pipe = IO.popen(cmd)
		pipe.close()
	end
end

puts(">> Generating thumbnails")
Dir[outdir + "/*.svg"].each do |f|
	puts(">>> " + File.basename(f))
	pipe1 = IO.popen("rsvg-convert --x-zoom=2.0 --y-zoom=2.0 #{f} -o #{f.gsub(".svg", ".png")}")
	pipe2 = IO.popen("rsvg-convert --x-zoom=0.7 --y-zoom=0.7 #{f} -o #{f.gsub(".svg", "-thumb.png")}")
	pipe1.close()
	pipe2.close()
end
