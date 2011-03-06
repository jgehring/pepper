--[[
	Generates a graph, representing lines of code over time.
--]]


-- Script meta-data
meta.title = "LOC"
meta.description = "Lines of code"
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with Lua pattern ARG"}}

require "pepper.plotutils"
pepper.plotutils.add_plot_options()


-- Revision callback function
function callback(r)
	if r:date() == 0 then return end

	s = r:diffstat()
	local delta = s:lines_added() - s:lines_removed()

	if locdeltas[r:date()] == nil then
		locdeltas[r:date()] = delta
	else
		locdeltas[r:date()] = locdeltas[r:date()] + delta
	end
	table.insert(dates, r:date())
end

-- Main report function
function main()
	dates = {}
	locdeltas = {}

	-- Gather data
	local repo = pepper.report.repository()
	local branch = pepper.report.getopt("b,branch", repo:default_branch())
	repo:iterator(branch):map(callback)

	-- Sort loc data by date
	table.sort(dates)
	local loc = {}
	local total = 0
	for k,v in ipairs(dates) do
		total = total + locdeltas[v]
		table.insert(loc, total)
	end

	-- Generate graph
	local p = pepper.gnuplot:new()
	pepper.plotutils.setup_output(p)
	pepper.plotutils.setup_std_time(p)
	p:set_title("Lines of Code (on " .. branch .. ")")

	if pepper.report.getopt("tags") ~= nil then
		pepper.plotutils.add_tagmarks(p, repo, pepper.report.getopt("tags", "*"))
	end

	p:set_xrange_time(dates[1], dates[#dates])
	p:plot_series(dates, loc)
end
