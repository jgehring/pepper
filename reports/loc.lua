--[[
-- Generate LOC and BOC graphs
--]]


-- Script meta-data
meta.name = "LOC"
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"-tARG, --type=ARG", "Select image type"}}

-- Revision callback function
function count(r)
	s = r:diffstat()
	t = s:files()
	for i,v in ipairs(t) do
		lines = lines + s:lines_added(v) - s:lines_removed(v)
		bytes = bytes + s:bytes_added(v) - s:bytes_removed(v)
	end
	if r:date() == 0 then
		return
	end
	-- TODO: Use dictionaries instead of arrays
	table.insert(loc, lines)
	table.insert(boc, bytes)
	table.insert(dates, r:date())
--	print(bytes, lines, r:date(), r:id())
end

-- Main report function
function main()
	loc = {}
	boc = {}
	dates = {}
	lines = 0
	bytes = 0

	-- Gather data
	branch = pepper.report.getopt("b, branch", pepper.report.repository():main_branch())
	pepper.report.walk_branch(count, branch)

	-- Generate graphs
	local setupcmd =  [[
set xdata time
set timefmt "%s"
set format x "%b %y"
set format y "%.0f"
set yrange [0:*]
set xtics nomirror
set xtics rotate by -45
set rmargin 8
set grid ytics]]
	local imgtype = pepper.report.getopt("t, type", "svg")
	local p = pepper.gnuplot:new()
	p:set_title("Lines of Code (on " .. branch .. ")")
	p:set_output("loc." .. imgtype)
	p:cmd(setupcmd)
	p:plot_series(dates, loc)

	local p2 = pepper.gnuplot:new()
	p2:set_title("Bytes of Code (on " .. branch .. ")")
	p2:set_output("boc." .. imgtype)
	p2:cmd(setupcmd)
	p2:plot_series(dates, boc)
end
