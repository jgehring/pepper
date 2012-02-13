--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: loc.lua
	Visualizes lines of code changes on a given branch.
--]]

require "pepper.datetime"
require "pepper.plotutils"


-- Describes the report
function describe(self)
	local r = {}
	r.name = "LOC"
	r.description = "Lines of code"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with Lua pattern ARG"}
	}
	pepper.datetime.add_daterange_options(r)
	pepper.plotutils.add_plot_options(r)
	return r
end

-- Revision callback function
function callback(r)
	if r:date() == 0 then return end

	s = r:diffstat()
	local delta = s:lines_added() - s:lines_removed()

	if locdeltas[r:date()] == nil then
		locdeltas[r:date()] = delta
		table.insert(dates, r:date())
	else
		locdeltas[r:date()] = locdeltas[r:date()] + delta
	end
end

-- Main report function
function run(self)
	dates = {}
	locdeltas = {}

	-- Gather data, but start at the beginning of the repository
	-- to get a proper LOC count.
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	local datemin, datemax = pepper.datetime.date_range(self)
	repo:iterator(branch, {stop=datemax}):map(callback)

	-- Sort loc data by date
	table.sort(dates)
	local loc = {}
	local keys = {}
	local total = 0
	for k,v in ipairs(dates) do
		total = total + locdeltas[v]
		if datemin < 0 or v >= datemin then
			table.insert(keys, v)
			table.insert(loc, total)
		end
	end

	-- Generate graph
	local p = pepper.gnuplot:new()
	pepper.plotutils.setup_output(p)
	pepper.plotutils.setup_std_time(p)
	p:set_title("Lines of Code (on " .. branch .. ")")

	if self:getopt("tags") ~= nil then
		pepper.plotutils.add_tagmarks(p, repo, self:getopt("tags", "*"))
	end

	p:set_xrange_time(keys[1], keys[#keys])
	p:plot_series(keys, loc)
end
