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
	local month = os.date("*t", r:date())["month"]
	commits[month] = commits[month] + 1
	changes[month] = changes[month] + #r:diffstat():files()
end

-- Main report function
function main()
	local time = os.time()
	local now = os.date("*t", time)
	commits = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	changes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

	-- Determine start of data collection: Twelve months ago
	local start = os.date("*t", time)
	if start["month"] == 12 then
		start["month"] = 1
	else
		start["year"] = start["year"] - 1
		start["month"] = start["month"] + 1
	end
	start["day"] = 1; start["hour"] = 0; start["min"] = 0; start["sec"] = 0

	-- Gather data
	local repo = pepper.report.repository()
	local branch = pepper.report.getopt("b,branch", repo:default_branch())
	repo:iterator(branch, os.time(start)):map(callback)

	-- Generate a data file
	local date = start
	date["month"] = (now["month"] % 12) + 1
	local keys = {}
	local values = {}
	while true do
		ncommits = commits[date["month"]]
		nchanges = changes[date["month"]]
		print(date["month"])

		-- Workaround: Place labels at every 2nd month only
		if #keys == 0 or keys[#keys] == "" then
			table.insert(keys, os.date("%B", os.time(date)))
		else
			table.insert(keys, "")
		end
		table.insert(values, {ncommits, nchanges})

		if date["month"] == now["month"] then
			break
		end
		date["month"] = (date["month"] % 12) + 1
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
