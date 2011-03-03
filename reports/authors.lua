--[[
	Plots a graph visualizing the code contribution by author
--]]


-- Script meta-data
meta.title = "Code contribution by authors"
meta.description = "Contributed lines of code by authors"
meta.graphical = true
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with a regular expression"},
                {"-nARG", "Show the ARG busiest authors"}}

-- Revision callback function
function callback(r)
	if r:author() == "" then
		return
	end

	-- Accumulate contributed lines
	local s = r:diffstat()
	local loc = 0
	for i,v in ipairs(s:files()) do
		loc = loc + s:lines_added(v)
	end

	-- Save commit and LOC count
	table.insert(commits, {r:date(), r:author(), loc})
	if authors[r:author()] == nil then
		authors[r:author()] = loc
	else
		authors[r:author()] = authors[r:author()] + loc
	end
end

-- Checks whether author a has more LOC than b
function authorcmp(a, b)
	return (a[2] > b[2])
end

-- Checks whether commit a has been earlier than b
function commitcmp(a, b)
	return (a[1] < b[1])
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

-- Main script function
function main()
	commits = {}   -- Commit list by timestamp with LOC delta
	authors = {}   -- Total LOC by author

	-- Gather data
	local repo = pepper.report.repository()
	local branch = pepper.report.getopt("b,branch", repo:default_branch())
	repo:iterator(branch):map(callback)

	-- Determine the 6 "busiest" authors (by LOC)
	local authorloc = {}
	for k,v in pairs(authors) do
		table.insert(authorloc, {k, v})
	end
	table.sort(authorloc, authorcmp)
	local i = 1 + tonumber(pepper.report.getopt("n", 6))
	while i <= #authorloc do
		authors[authorloc[i][1]] = nil
		authorloc[i] = nil
		i = i + 1
	end

	-- Sort commits by time
	table.sort(commits, commitcmp)

	-- Generate data arrays for the authors
	local keys = {}
	local series = {}
	local loc = {}
	for i,a in ipairs(authorloc) do
		loc[a[1]] = 0
	end
	for t,v in ipairs(commits) do
		table.insert(keys, v[1])
		table.insert(series, {});
		for i,a in ipairs(authorloc) do
			if a[1] == v[2] then
				loc[a[1]] = loc[a[1]] + v[3]
			end
			table.insert(series[#series], loc[a[1]])
		end
	end

	local authors = {}
	for i,a in ipairs(authorloc) do
		table.insert(authors, a[1])
	end

	local p = pepper.gnuplot:new()
	p:setup(600, 480)
	p:set_title("Contributed Lines of Code by Author (on " .. branch .. ")")

	if pepper.report.getopt("tags") ~= nil then
		add_tagmarks(p)
	end

	p:cmd([[
set xdata time
set timefmt "%s"
set format x "%b %y"
set format y "%'.0f"
set yrange [0:*]
set xtics nomirror
set xtics rotate by -45
set grid ytics
set grid x2tics
set rmargin 8
set key box
set key below
]])
	p:set_xrange_time(keys[1], keys[#keys])
	p:plot_series(keys, series, authors)
end
