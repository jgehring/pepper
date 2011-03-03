--[[
	Generates a graph, representing lines of code over time.
--]]


-- Script meta-data
meta.title = "LOC"
meta.description = "Lines of code"
meta.graphical = true
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with Lua pattern ARG"}}


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

-- Convert from UNIX to Gnuplot epoch
function convepoch(t)
	return t - 946684800
end

-- Adds x2tics for repository tags
function add_tagmarks(plot)
	-- Fetch tags and generate tic data
	local repo = pepper.report.repository()
	local regex = pepper.report.getopt("tags", "*")
	local tags = repo:tags()
	local x2tics = ""
	if #tags > 0 then
		x2tics = "("
		for k,v in ipairs(tags) do
			if v:name():find(regex) ~= nil then
				x2tics = x2tics .. "\"" .. v:name() .. "\" " .. convepoch(repo:revision(v:id()):date()) .. ","
			end
		end
		if #x2tics == 1 then
			return
		end
		x2tics = x2tics:sub(0, #x2tics-1) .. ")"
		plot:cmd("set x2data time")
		plot:cmd("set format x2 \"%s\"")
		plot:cmd("set x2tics scale 0")
		plot:cmd("set x2tics " .. x2tics)
		plot:cmd("set x2tics border rotate by 60")
		plot:cmd("set x2tics font \"Helvetica,8\"")
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
		add_tagmarks(p)
	end

	-- Generate graph
	p:cmd([[
set xdata time
set timefmt "%s"
set format x "%b %y"
set format y "%'.0f"
set yrange [0:*]
set xtics nomirror
set xtics rotate by -45
set rmargin 8
set grid ytics
set grid x2tics
]])
	p:set_xrange_time(dates[1], dates[#dates])
	p:plot_series(dates, loc)
end
