--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: diffstat.lua
	Visualizes directory size changes on a given branch.
--]]

require "pepper.datetime"
require "pepper.plotutils"


-- Describes the report
function describe(self)
	local r = {}
	r.title = "Directories"
	r.description = "Directory sizes"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with a regular expression"},
		{"-nARG", "Show the ARG largest directories"}
	}
	pepper.datetime.add_daterange_options(r)
	pepper.plotutils.add_plot_options(r)
	return r
end

-- Revision callback function
function count(r)
	local s = r:diffstat()

	-- Save commit and file paths
	table.insert(commits, {r:date(), pepper.diffstat(s)})

	-- Update directory sizes
	for i,v in ipairs(s:files()) do
		local dir = pepper.utils.dirname("/" .. v)
		local old = 0
		if directories[dir] == nil then
			directories[dir] = s:lines_added(v) - s:lines_removed(v)
		else
			old = directories[dir]
			directories[dir] = directories[dir] + s:lines_added(v) - s:lines_removed(v)
		end
	end
end

-- Compares directory sizes of a and b
function dircmp(a, b)
	return (a[2] > b[2])
end

-- Checks whether commit a has been earlier than b
function commitcmp(a, b)
	return (a[1] < b[1])
end

-- Main report function
function main(self)
	commits = {}     -- Commit list by timestamp with diffstat
	directories = {} -- Total LOC by directory

	-- Gather data, but start at the beginning of the repository
	-- to get a proper LOC count.
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	local datemin, datemax = pepper.datetime.date_range(self)
	repo:iterator(branch, {stop=datemax}):map(count)

	-- Determine the largest directories (by current LOC)
	local dirloc = {}
	for k,v in pairs(directories) do
		table.insert(dirloc, {k, v})
	end
	table.sort(dirloc, dircmp)
	local i = 1 + tonumber(self:getopt("n", 6))
	while i <= #dirloc do
		directories[dirloc[i][1]] = nil
		dirloc[i] = nil
		i = i + 1
	end

	-- Sort commits by time
	table.sort(commits, commitcmp)

	-- Generate data arrays for the directories, skipping data points
	-- prior to datemin.
	local keys = {}
	local series = {}
	local loc = {}
	for i,a in ipairs(dirloc) do
		loc[a[1]] = 0
	end
	for t,v in ipairs(commits) do
		if datemin < 0 or v[1] >= datemin then
			table.insert(keys, v[1])
			table.insert(series, {});
		end

		-- Update directory sizes
		local s = v[2];
		for i,v in ipairs(s:files()) do
			local dir = pepper.utils.dirname("/" .. v)
			if loc[dir] ~= nil then
				loc[dir] = loc[dir] + s:lines_added(v) - s:lines_removed(v)
			end
		end

		if datemin < 0 or v[1] >= datemin then
			for i,a in ipairs(dirloc) do
				table.insert(series[#series], loc[a[1]])
			end
		end
	end

	local directories = {}
	for i,a in ipairs(dirloc) do
		if #a[1] > 1 then
			table.insert(directories, a[1]:sub(2))
		else
			table.insert(directories, a[1])
		end
	end

	local p = pepper.gnuplot:new()
	pepper.plotutils.setup_output(p)
	pepper.plotutils.setup_std_time(p, {key = "below"})
	p:set_title("Directory Sizes (on " .. branch .. ")")

	if self:getopt("tags") ~= nil then
		pepper.plotutils.add_tagmarks(p, repo, self:getopt("tags", "*"))
	end

	p:set_xrange_time(keys[1], keys[#keys])
	p:plot_series(keys, series, directories)
end
