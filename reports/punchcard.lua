--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: punchcard.lua
	Commit activity, as display by Github (https://github.com/blog/159-one-more-thing)
	or "git timecard" (http://dustin.github.com/2009/01/11/timecard.html)
--]]

require "pepper.datetime"
require "pepper.plotutils"


-- Describes the report
function describe(self)
	local r = {}
	r.title = "Punchcard"
	r.description = "Commit activity by day and hour"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"-aARG, --author=ARG", "Show commits of author ARG"}
	}
	pepper.datetime.add_daterange_options(r)
	pepper.plotutils.add_plot_options(r)
	return r
end

-- Revision callback function
function callback(r)
	if r:date() == 0 then
		return
	end

	if author and r:author() ~= author then
		return
	end

	local date = os.date("%w %H", r:date())
	if commits[date] == nil then
		commits[date] = 1
	else
		commits[date] = commits[date] + 1
	end
	if commits[date] > max then
		max = commits[date]	
	end
end

-- Main report function
function run(self)
	commits = {}  -- Number of commits: (hour, wday) -> num
	author = self:getopt("a,author")
	max = 0

	-- Gather data
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	local datemin, datemax = pepper.datetime.date_range(self)
	repo:iterator(branch, {start=datemin, stop=datemax}):map(callback)

	max = max * 2

	-- Extract data
	local hours = {}
	local days_nums = {}
	for k,v in pairs(commits) do
		local a,b,day,hour = string.find(k, "(%d+) (%d+)")
		table.insert(hours, hour)
		table.insert(days_nums, {{day, v / max}}) -- Two values for a single series
	end

	-- Generate graph
	local p = pepper.gnuplot:new()
	pepper.plotutils.setup_output(p, 800, 300)
	if author then
		p:set_title("Commit Activity by Day and Hour (by " .. author .. " on " .. branch .. ")")
	else
		p:set_title("Commit Activity by Day and Hour (on " .. branch .. ")")
	end

	p:cmd([[
set xrange [-1:24]
set xtics 0,1, 23
set yrange [-1:7]
set ytics ("Sun" 0, "Mon" 1, "Tue" 2, "Wed" 3, "Thu" 4, "Fri" 5, "Sat" 6)
set nokey
]])
	p:plot_series(hours, days_nums, {}, {command = "with circles lc rgb \"black\" fs solid noborder"});
end
