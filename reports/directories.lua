--[[
	Generates a graph, representing directory sizes over time.
--]]


-- Script meta-data
meta.title = "Directories"
meta.description = "Directory sizes"
meta.graphical = true
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with a regular expression"},
                {"-nARG", "Show the ARG largest directories"}}

require "pepper.plotutils"


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
	local repo = pepper.report.repository()
	local branch = pepper.report.getopt("b, branch", repo:default_branch())
	repo:iterator(branch):map(count)

	-- Determine the largest directories (by current LOC)
	local dirloc = {}
	for k,v in pairs(directories) do
		table.insert(dirloc, {k, v})
	end
	table.sort(dirloc, dircmp)
	local i = 1 + tonumber(pepper.report.getopt("n", 6))
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
	p:setup(600, 480)
	p:set_title("Directory Sizes (on " .. branch .. ")")

	if pepper.report.getopt("tags") ~= nil then
		pepper.plotutils.add_tagmarks(p, repo, pepper.report.getopt("tags", "*"))
	end

	pepper.plotutils.setup_std_time(p, {key = "below"})
	p:set_xrange_time(keys[1], keys[#keys])
	p:plot_series(keys, series, directories)
end
