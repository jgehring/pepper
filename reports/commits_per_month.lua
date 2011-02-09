--[[
	Generates a histogram using commits per month and the amount of files touched.
--]]


-- Script meta-data
meta.title = "Commits per Month"
meta.description = "Histogramm of commit counts during the last twelve months"
meta.graphical = true
meta.options = {{"-bARG, --branch=ARG", "Select branch"}}

-- Revision callback function
function callback(r)
	local date = os.date("*t", r:date())

	if commits[date["month"]] == nil then
		commits[date["month"]] = 0
		changes[date["month"]] = 0
	end
	commits[date["month"]] = commits[date["month"]] + 1
	changes[date["month"]] = changes[date["month"]] + #r:diffstat():files()
end

-- Main report function
function main()
	local nowt = os.time()
	now = os.date("*t", nowt)
	commits = {}
	changes = {}

	-- Determine start of data collection: Twelve months ago
	local start = os.date("*t", nowt)
	if start["month"] == 12 then
		start["month"] = 1
	else
		start["year"] = start["year"] - 1
		start["month"] = start["month"] + 1
	end
	start["day"] = 1
	start["hour"] = 0
	start["min"] = 0
	start["sec"] = 0

	-- Gather data
	local repo = pepper.report.repository()
	local branch = pepper.report.getopt("b,branch", repo:default_branch())
	repo:walk_branch(callback, branch, os.time(start))

	-- Generate a data file
	local names = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"}
	local month = (now["month"] % 12) + 1
	local keys = {}
	local values = {}
	local i = 1
	while true do
		ncommits = 0
		nchanges = 0
		if commits[month] ~= nil then
			ncommits = commits[month]
			nchanges = changes[month]
		end

		-- Workaround: Place labels at every 2nd month only
		if (i % 2) == 1 then
			table.insert(keys, names[month])
		else
			table.insert(keys, "")
		end
		table.insert(values, {ncommits, nchanges})

		if month == now["month"] then
			break
		end
		month = (month % 12) + 1
		i = i + 1
	end

	-- Generate graph
	local p = pepper.gnuplot:new()
	p:setup(800, 480)
	p:set_title("Commits per Month (on " .. branch .. ")")

	p:cmd([[
set format y "%.0f"
set yrange [0:*]
set grid ytics
set ytics scale 0 
set key box
set key below
set xtics nomirror
set style fill solid border -1
set style histogram cluster gap 1
]])
	p:plot_histogram(keys, values, {"Commits", "Changes"})
end
