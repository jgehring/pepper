--[[
	Generates a graph, representing Lines of Code over time.
--]]


-- Script meta-data
meta.title = "LOC"
meta.description = "Lines of code"
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"-tARG, --type=ARG", "Select image type"}}

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
	branch = pepper.report.getopt("b, branch", pepper.report.repository():main_branch())
	pepper.report.walk_branch(count, branch)

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

	-- Generate graphs
	local imgtype = pepper.report.getopt("t, type", "svg")
	local p = pepper.gnuplot:new()
	p:set_title("Lines of Code (on " .. branch .. ")")
	p:set_output("loc." .. imgtype)
	p:cmd([[
set xdata time
set timefmt "%s"
set format x "%b %y"
set decimal locale
set format y "%'.0f"
set yrange [0:*]
set xtics nomirror
set xtics rotate by -45
set rmargin 8
set grid ytics]])
	p:plot_series(dates, loc)
end
