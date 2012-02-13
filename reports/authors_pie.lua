--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: authors_pie.lua
	Commit or change percentage by author.
--]]

require "pepper.datetime"
require "pepper.plotutils"


-- Describes the report
function describe(self)
	local r = {}
	r.name = "Author contributions"
	r.description = "Commit or change percentage by authors"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with a regular expression"},
		{"-nARG", "Show the ARG busiest authors"},
		{"-c, --changes, -l", "Count line changes instead of commit counts"}
	}
	pepper.datetime.add_daterange_options(r)
	pepper.plotutils.add_plot_options(r)
	return r
end

-- Revision callback function
function callback(r)
	if r:author() == "" then
		return
	end

	local loc = r:diffstat():lines_added()
	if not count_lines then
		loc = 1 -- Count commits only
	end

	if authors[r:author()] == nil then
		authors[r:author()] = loc
	else
		authors[r:author()] = authors[r:author()] + loc
	end
end

-- Main script function
function run(self)
	authors = {}   -- Total LOC by author

	-- Gather data, but 	count_lines = self:getopt("c,changes,l")
	count_lines = self:getopt("c,changes,l")
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	local datemin, datemax = pepper.datetime.date_range(self)
	if count_lines then
		-- Start at the beginning of the repository to get a proper LOC count.
		repo:iterator(branch, {stop=datemax}):map(callback)
	else
		repo:iterator(branch, {start=datemin, stop=datemax}):map(callback)
	end

	-- Sort authors by contribution and sum up total contribution
	local authorloc = {}
	local total = 0
	for k,v in pairs(authors) do
		table.insert(authorloc, {k, v})
		total = total + v
	end

	-- Sort by contribution, so we can select the busiest ones
	table.sort(authorloc, function (a,b) return (a[2] > b[2]) end)

	local i = 1 + tonumber(self:getopt("n", 6))
	local missing = 0
	while i <= #authorloc do
		authors[authorloc[i][1]] = nil
		authorloc[i] = nil
		i = i + 1
	end

	-- Prepare series with percentage values
	local keys = {}
	local values = {}
	local sum = 0
	for i,a in ipairs(authorloc) do
		table.insert(keys, a[1])
		table.insert(values, a[2] / total)
		sum = sum + values[#values]
	end
	if sum < 0.99999 then
		table.insert(keys, "Others")
		table.insert(values, 1.0 - sum)
	end

	local p = pepper.gnuplot:new()
	pepper.plotutils.setup_output(p, 600, 600)
	if count_lines then
		p:set_title("Percentage of contributed code by author (on " .. branch .. ")")
	else
		p:set_title("Percentage of total commits by author (on " .. branch .. ")")
	end
	p:plot_pie(keys, values)
end
