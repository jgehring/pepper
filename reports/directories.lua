--[[
	Generates a graph, representing directory sizes over time.
--]]


-- Script meta-data
meta.title = "Directories"
meta.description = "Directory sizes"
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"-tARG, --type=ARG", "Select image type"},
                {"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with a regular expression"}}

-- Returns the dirname() of a file
function dirname(filename) 
	local dir,n = string.gsub(filename, "(.*/)(.*)", "%1")
	if n == 0 then
		return "/"
	else
		return "/" .. dir
	end
end 

-- Revision callback function
function count(r)
	local s = r:diffstat()

	-- Save commit and file paths
	table.insert(commits, {r:date(), pepper.diffstat(s)})

	-- Update directory sizes
	for i,v in ipairs(s:files()) do
		local dir = dirname(v)
		local old = 0
		if directories[dir] == nil then
			directories[dir] = s:lines_added(v) - s:lines_removed(v)
		else
			old = directories[dir]
			directories[dir] = directories[dir] + s:lines_added(v) - s:lines_removed(v)
		end
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

-- Compares directory sizes of a and b
function dircmp(a, b)
	return (a[2] > b[2])
end

-- Checks whether commit a has been earlier than b
function commitcmp(a, b)
	return (a[1] < b[1])
end

-- Main report function
function main()
	commits = {}     -- Commit list by timestamp with diffstat
	directories = {} -- Total LOC by directory

	-- Gather data
	branch = pepper.report.getopt("b, branch", pepper.report.repository():main_branch())
	pepper.report.walk_branch(count, branch)

	-- Determine the largest directories (by current LOC)
	local dirloc = {}
	for k,v in pairs(directories) do
		table.insert(dirloc, {k, v})
	end
	table.sort(dirloc, dircmp)
	local i = 7
	while i <= #dirloc do
		directories[dirloc[i][1]] = nil
		dirloc[i] = nil
		i = i + 1
	end

	-- Sort commits by time
	table.sort(commits, commitcmp)

	-- Generate data arrays for the directories
	local keys = {}
	local series = {}
	local loc = {}
	for i,a in ipairs(dirloc) do
		loc[a[1]] = 0
	end
	for t,v in ipairs(commits) do
		table.insert(keys, v[1])
		table.insert(series, {});

		-- Update directory sizes
		local s = v[2];
		for i,v in ipairs(s:files()) do
			local dir = dirname(v)
			if loc[dir] ~= nil then
				loc[dir] = loc[dir] + s:lines_added(v) - s:lines_removed(v)
			end
		end

		for i,a in ipairs(dirloc) do
			table.insert(series[#series], loc[a[1]])
		end
	end

	local directories = {}
	for i,a in ipairs(dirloc) do
		if #a[1] > 1 then
			table.insert(directories, a[1]:sub(2))
		else
			table.insert(directories, a[1])
		end
	end

	local p = pepper.gnuplot:new()
	p:set_title("Directory sizes (on " .. branch .. ")")
	p:set_output("directories." .. pepper.report.getopt("t, type", "svg"), 600, 480)

	if pepper.report.getopt("tags") ~= nil then
		add_tagmarks(p)
	end

	-- Determine time range: 5% spacing at start and end, rounded to 1000 seconds
	-- This is imporant for aligning the xaxis and x2axis (when using tags)
	local range = keys[#keys] - keys[1]
	local tstart = convepoch(1000 * math.floor((keys[1] - 0.05 * range) / 1000))
	local tend = convepoch(1000 * math.ceil((keys[#keys] + 0.05 * range) / 1000))

	p:cmd([[
set xdata time
set timefmt "%s"
set format x "%b %y"
set decimal locale
set format y "%'.0f"
set yrange [0:*]
set xtics nomirror
set xtics rotate by -45
set grid ytics
set grid x2tics
set rmargin 8
set key box
set key below]])
	p:cmd("set xrange [" .. tstart .. ":" .. tend .. "]")
	p:cmd("set x2range [" .. tstart .. ":" .. tend .. "]")
	p:plot_series(keys, series, directories)
end
