--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: times.lua
	Visualizes commit times using a scatter plot.
--]]

require "pepper.datetime"
require "pepper.plotutils"


-- Describes the report
function describe(self)
	local r = {}
	r.title = "Times"
	r.description = "Scatter plot of commit times"
	r.options = {{"-bARG, --branch=ARG", "Select branch"}}
	pepper.datetime.add_daterange_options(r)
	pepper.plotutils.add_plot_options(r)
	return r
end

-- Revision callback function
function callback(r)
	if r:date() == 0 then
		return
	end

	local date = os.date("*t", r:date())
	table.insert(dates, r:date())
	table.insert(daytimes, date["hour"] + date["min"] / 60)

	-- Track date range
	if r:date() < firstdate then firstdate = r:date() end
	if r:date() > lastdate then lastdate = r:date() end
end

-- Main report function
function run(self)
	dates = {}     -- Commit timestamps
	daytimes = {}  -- Time in hours and hour fractions

	-- Date range
	firstdate = os.time()
	lastdate = 0

	-- Gather data
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	local datemin, datemax = pepper.datetime.date_range(self)
	repo:iterator(branch, {start=datemin, stop=datemax}):map(callback)

	-- Generate graph
	local p = pepper.gnuplot:new()
	pepper.plotutils.setup_output(p, 640, 240)
	pepper.plotutils.setup_std_time(p)
	p:set_title("Commit Times (on " .. branch .. ")")

	p:cmd([[
set yrange [0:24]
set ytics 6
set grid
set pointsize 1
]])
	p:set_xrange_time(firstdate, lastdate)
	p:plot_series(dates, daytimes, {}, "points")
end
