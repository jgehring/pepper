--[[
	Generates a graph, representing lines of code over time.
--]]


-- Script meta-data
meta.title = "LOC"
meta.description = "Lines of code"
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with a regular expression"},
                {"-oARG, --output=ARG", "Select output file (defaults to stdout)"},
                {"-tARG, --type=ARG", "Explicitly set image type"},
                {"-sW[xH], --size=W[xH]", "Set image size to width W and height H"}}

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
				x2tics = x2tics .. "\"" .. v:name() .. "\" " .. convepoch(pepper.report.revision(v:id()):date()) .. ","
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

-- Sets up the plot according to the command line arguments
function setup_plot(branch)
	local p = pepper.gnuplot:new()
	p:set_title("Lines of Code (on " .. branch .. ")")

	local file = pepper.report.getopt("o, output", "")
	local size = pepper.utils.split(pepper.report.getopt("s, size", "600"), "x")
	local terminal = pepper.report.getopt("t, type", "svg")
	local width = tonumber(size[1])
	local height = width * 0.8
	if (#size > 1) then
		height = tonumber(size[2])
	end

	p:set_output(file, width, height, terminal)
	return p
end

-- Main report function
function main()
	locdeltas = {}

	-- Gather data
	local branch = pepper.report.getopt("b, branch", pepper.report.repository():main_branch())
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

	local p = setup_plot(branch)

	if pepper.report.getopt("tags") ~= nil then
		add_tagmarks(p)
	end

	-- Determine time range: 5% spacing at start and end, rounded to 1000 seconds
	-- This is imporant for aligning the xaxis and x2axis (when using tags)
	local range = dates[#dates] - dates[1]
	local tstart = convepoch(1000 * math.floor((dates[1] - 0.05 * range) / 1000))
	local tend = convepoch(1000 * math.ceil((dates[#dates] + 0.05 * range) / 1000))

	-- Generate graphs
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
set grid ytics
set grid x2tics
]])
	p:cmd("set xrange [" .. tstart .. ":" .. tend .. "]")
	p:cmd("set x2range [" .. tstart .. ":" .. tend .. "]")
	p:plot_series(dates, loc)
end
