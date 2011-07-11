--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2011 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: authors.lua
	Visualizes code contribution by author.
--]]

require "pepper.plotutils"


-- Describes the report
function describe(self)
	local r = {}
	r.name = "Authors"
	r.description = "Contributed lines of code by authors"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with a regular expression"},
		{"-nARG", "Show the ARG busiest authors"}
	}
	pepper.plotutils.add_plot_options(r)
	return r
end

-- Revision callback function
function callback(r)
	if r:author() == "" then
		return
	end


	-- Save commit and LOC count
	local loc = r:diffstat():lines_added()
	table.insert(commits, {r:date(), r:author(), loc})
	if authors[r:author()] == nil then
		authors[r:author()] = loc
	else
		authors[r:author()] = authors[r:author()] + loc
	end
end

-- Main script function
function run(self)
	commits = {}   -- Commit list by timestamp with LOC delta
	authors = {}   -- Total LOC by author

	-- Gather data
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	repo:iterator(branch):map(callback)

	-- Determine the "busiest" authors (by LOC)
	local authorloc = {}
	for k,v in pairs(authors) do
		table.insert(authorloc, {k, v})
	end
	table.sort(authorloc, function (a,b) return (a[2] > b[2]) end)
	local i = 1 + tonumber(self:getopt("n", 6))
	while i <= #authorloc do
		authors[authorloc[i][1]] = nil
		authorloc[i] = nil
		i = i + 1
	end

	-- Sort commits by time
	table.sort(commits, function (a,b) return (a[1] < b[1]) end)

	-- Generate data arrays for the authors
	local keys = {}
	local series = {}
	local loc = {}
	for i,a in ipairs(authorloc) do
		loc[a[1]] = 0
	end
	for t,v in ipairs(commits) do
		table.insert(keys, v[1])
		table.insert(series, {});
		for i,a in ipairs(authorloc) do
			if a[1] == v[2] then
				loc[a[1]] = loc[a[1]] + v[3]
			end
			table.insert(series[#series], loc[a[1]])
		end
	end

	local authors = {}
	for i,a in ipairs(authorloc) do
		table.insert(authors, a[1])
	end

	local p = pepper.gnuplot:new()
	pepper.plotutils.setup_output(p)
	pepper.plotutils.setup_std_time(p, {key = "below"})
	p:set_title("Contributed Lines of Code by Author (on " .. branch .. ")")

	if self:getopt("tags") ~= nil then
		pepper.plotutils.add_tagmarks(p, repo, self:getopt("tags", "*"))
	end

	p:set_xrange_time(keys[1], keys[#keys])
	p:plot_series(keys, series, authors)
end
