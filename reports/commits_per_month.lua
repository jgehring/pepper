--[[
	Generates a histogram using commits per month and the amount of files touched.
--]]


-- Script meta-data
meta.title = "Commits per Month"
meta.description = "Histogramm of commit counts during the last twelve months"
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"-oARG, --output=ARG", "Select output file (defaults to stdout)"},
                {"-tARG, --type=ARG", "Explicitly set image type"},
                {"-sW[xH], --size=W[xH]", "Set image size to width W and height H"}}

-- Revision callback function
function callback(r)
	local date = os.date("*t", r:date())
	-- Ignore commits older than 1 year
	if (now["year"] - date["year"] > 1) or (now["year"] - date["year"] == 1 and date["month"] <= now["month"]) then
		return
	end

	if commits[date["month"]] == nil then
		commits[date["month"]] = 0
		changes[date["month"]] = 0
	end
	commits[date["month"]] = commits[date["month"]] + 1
	changes[date["month"]] = changes[date["month"]] + #r:diffstat():files()
end

-- Sets up the plot according to the command line arguments
function setup_plot(branch)
	local p = pepper.gnuplot:new()
	p:set_title("Commits per Month (on " .. branch .. ")")

	local file = pepper.report.getopt("o, output", "")
	local size = pepper.utils.split(pepper.report.getopt("s, size", "800"), "x")
	local terminal = pepper.report.getopt("t, type")
	local width = tonumber(size[1])
	local height = width * 0.6
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
	now = os.date("*t", os.time())
	commits = {}
	changes = {}

	-- Gather data
	local branch = pepper.report.getopt("b,branch", pepper.report.repository():main_branch())
	-- TODO: Restrict to last twelve months
	pepper.report.walk_branch(callback, branch)

	-- Generate a data file
	local names = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"}
	local month = (now["month"] % 12) + 1
	local i = 1
	local file, filename = pepper.utils.mkstemp()
	while true do
		ncommits = 0
		nchanges = 0
		if commits[month] ~= nil then
			ncommits = commits[month]
			nchanges = changes[month]
		end
		file:write(i .. " " .. names[month] .. " " .. ncommits .. " " .. nchanges .. "\n")
		if month == now["month"] then
			break
		end
		month = (month % 12) + 1
		i = i + 1
	end

	-- Generate graphs
	local plot = pepper.gnuplot:new()
	local p = setup_plot(branch)
	p:cmd([[
set format y "%.0f"
set yrange [0:*]
set grid ytics
set ytics scale 0 
set key box
set key below
set boxwidth 0.4
set xtics nomirror
]])

	-- Determine xtics
	month = (now["month"] % 12) + 1
	i = 1
	local xtics = ""
	while true do
		if month == now["month"] then
			break
		end
		if i % 2 ~= 0 then
			xtics = xtics .. '"' .. names[month] .. "\" " .. i .. ","
		end
		month = (month % 12) + 1
		i = i + 1
	end
	p:cmd("set xtics (" .. xtics:sub(0, #xtics-1) .. ") scale 0")

	local cmd = "plot "
	cmd = cmd .. "\"" .. filename .. "\" using 1:3 with boxes fs solid title \"Commits\", "
	cmd = cmd .. "\"" .. filename .. "\" using ($1+.4):4 with boxes fs solid title \"Changes\";"

	file:close()
	p:cmd(cmd)
	p:flush()
	pepper.utils.unlink(filename)
end
