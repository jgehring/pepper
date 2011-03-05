--[[
	Generates a graph, representing lines of code over time.
--]]


-- Script meta-data
meta.title = "LOC"
meta.description = "Lines of code"
meta.graphical = true
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with Lua pattern ARG"}}

require "pepper.plotutils"


-- Revision callback function
function count(r)
	if r:date() == 0 then
		return
	end

	s = r:diffstat()
	local delta = s:lines_added() - s:lines_removed()

	if locdeltas[r:date()] == nil then
		locdeltas[r:date()] = delta
	else
		locdeltas[r:date()] = locdeltas[r:date()] + delta
	end
end

-- Main report function
function main()
	locdeltas = {}

	-- Gather data
	local repo = pepper.report.repository()
	local branch = pepper.report.getopt("b, branch", repo:default_branch())
	repo:iterator(branch):map(count)

	-- Sort loc data by date
	local dates = {}
	local loc = {}
	for k,v in pairs(locdeltas) do
		table.insert(dates, k)
	end
	table.sort(dates)

	local total = 0
	for k,v in ipairs(dates) do
		total = total + locdeltas[v]
		table.insert(loc, total)
	end

	p = pepper.gnuplot:new()
	p:setup(600, 480)
	p:set_title("Lines of Code (on " .. branch .. ")")

	if pepper.report.getopt("tags") ~= nil then
		pepper.plotutils.add_tagmarks(p, repo, pepper.report.getopt("tags", "*"))
	end

	-- Generate graph
	pepper.plotutils.setup_std_time(p)
	p:set_xrange_time(dates[1], dates[#dates])
	p:plot_series(dates, loc)
end
