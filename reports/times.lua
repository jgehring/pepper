--[[
	Generates a scatter plot visualizing the commit activity per daytime.
--]]


-- Script meta-data
meta.title = "Commit Scatter"
meta.description = "Scatter plot of commit activity"
meta.options = {{"-bARG, --branch=ARG", "Select branch"}}

require "pepper.plotutils"
pepper.plotutils.add_plot_options()

-- Revision callback function
function callback(r)
	if r:date() == 0 then
		return
	end

	local date = os.date("*t", r:date())
	table.insert(dates, r:date())
	table.insert(daytimes, date["hour"] + date["min"] / 60)
end

-- Main report function
function main()
	dates = {}     -- Commit timestamps
	daytimes = {}  -- Time in hours and hour fractions

	-- Gather data
	local repo = pepper.report.repository()
	local branch = pepper.report.getopt("b,branch", repo:default_branch())
	repo:iterator(branch):map(callback)

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
	p:set_xrange_time(dates[1], dates[#dates])
	p:plot_series(dates, daytimes, {}, "dots")
end
