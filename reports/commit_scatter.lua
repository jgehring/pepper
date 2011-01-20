--[[
	Generates a scatter plot visualizing the commit activity per daytime.
--]]


-- Script meta-data
meta.title = "Commit Scatter"
meta.description = "Scatter plot of commit activity"
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"-oARG, --output=ARG", "Select output file (defaults to stdout)"},
                {"-tARG, --type=ARG", "Explicitly set image type"},
                {"-sW[xH], --size=W[xH]", "Set image size to width W and height H"}}

-- Revision callback function
function callback(r)
	if r:date() == 0 then
		return
	end

	local date = os.date("*t", r:date())
	table.insert(dates, r:date())
	table.insert(daytimes, date["hour"] + date["min"] / 60)
end

-- Sets up the plot according to the command line arguments
function setup_plot(branch)
	local p = pepper.gnuplot:new()
	p:set_title("Commit Activity (on " .. branch .. ")")

	local file = pepper.report.getopt("o, output", "")
	local size = pepper.utils.split(pepper.report.getopt("s, size", "600"), "x")
	local terminal = pepper.report.getopt("t, type")
	local width = tonumber(size[1])
	local height = width / 3.0
	if (#size > 1) then
		height = tonumber(size[2])
	end

	if terminal ~= nil then
		p:set_output(file, width, height, terminal)
	else
		p:set_output(file, width, height)
	end
	return p
end

-- Main report function
function main()
	dates = {}     -- Commit timestamps
	daytimes = {}  -- Time in hours and hour fractions

	-- Gather data
	local branch = pepper.report.getopt("b,branch", pepper.report.repository():main_branch())
	pepper.report.walk_branch(callback, branch)

	-- Generate graph
	local p = setup_plot(branch)
	p:cmd([[
set xdata time
set timefmt "%s"
set format x "%b %y"
set format y "%'.0f"
set yrange [0:24]
set ytics 6
set xtics nomirror
set xtics rotate by -45
set rmargin 8
set grid
set pointsize 0.1]])
	p:plot_series(dates, daytimes, {}, "dots")
end
