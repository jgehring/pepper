--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: activity.lua
	Visualizes several repository activity metrics using a smoothed curve.
	This is a port of http://labs.freehackers.org/wiki/hgactivity
--]]

require "pepper.datetime"
require "pepper.plotutils"


-- Describes the report
function describe(self)
	local r = {}
	r.name = "Activity"
	r.description = "General repository activity"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"--tags[=ARG]", "Add tag markers to the graph, optionally filtered with a regular expression"},
		{"-c, --changes, -l", "Count line changes instead of commit counts"},
		{"--split=ARG", "Split the graph by 'authors', 'files', 'directories', or 'none' (the default)"},
		{"-nARG", "Maximum number of series when using --split"}
	}
	pepper.datetime.add_daterange_options(r)
	pepper.plotutils.add_plot_options(r)
	return r
end

-- Revision callback function
function count(r)
	local date = r:date()
	if date == 0 then return end

	local n = 1
	if count_changes ~= nil then
		n = r:diffstat():lines_added() + r:diffstat():lines_removed()
	end
	if split == "none" then
		if activity[date] == nil then activity[date] = n 
		else activity[date] = activity[date] + n end
	elseif split == "authors" then
		if activity[r:author()] == nil then activity[r:author()] = {} end
		if activity[r:author()][date] == nil then activity[r:author()][date] = n 
		else activity[r:author()][date] = activity[r:author()][date] + n end
	elseif split == "directories" or split == "files" then
		local s = r:diffstat()
		local dir = (split == "directories")
		for i,v in ipairs(s:files()) do
			if count_changes ~= nil then
				n = s:lines_added(v) + s:lines_removed(v)
			end
			if dir then v = pepper.utils.dirname("/" .. v) end
			if activity[v] == nil then activity[v] = {} end
			if activity[v][date] == nil then
				activity[v][date] = n
			else
				activity[v][date] = activity[v][date] + n
			end
		end
	end

	-- Track date range
	if date < firstdate then firstdate = date end
	if date > lastdate then lastdate = date end
end

-- This is mostly from hg's activity extension
function convolution(datemin, datemax, data)
	local date = datemin
	local wmin = 1
	local wmax = 1
	local number = 1000 -- number of points we want to compute
	local period = (datemax-datemin)/number -- period at which we compute a value
	local wperiod = period * 25 -- length of the convolution window
	local dates = {}
	local values = {}

	local mydates = {}
	for k,v in pairs(data) do
		table.insert(mydates, k)
	end
	table.sort(mydates)
	local length = #mydates
	for x = 0, number do
		date = date + period
		while wmin <= length and mydates[wmin] < date - wperiod do
			wmin = wmin + 1
		end
		while wmax <= length and mydates[wmax] < date + wperiod do
			wmax = wmax + 1
		end
		value = 0
		for a = wmin, wmax-1 do
			local delta = mydates[a] - date
			value = value + data[mydates[a]] * (1 - delta/wperiod)
		end
		table.insert(values, value)
		table.insert(dates, date)
	end

	return dates, values
end

-- Returns the size of a table
function tablesize(t)
	local n = 0
	for k,v in pairs(t) do
		n = n + 1
	end
	return n
end

-- Main report function
function run(self)
	activity = {}
	firstdate = os.time()
	lastdate = 0

	count_changes = self:getopt("c,changes,l")
	split = self:getopt("split", "none")
	if split == "n" then split="none"
	elseif split == "a" then split="authors"
	elseif split == "f" then split="files"
	elseif split == "d" then split="directories" end
	if ({none=1, authors=1, files=1, branches=1, directories=1})[split] == nil then
		error("Unknown split option '" .. split .. "'")
	end

	-- Gather data
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	local datemin, datemax = pepper.datetime.date_range(self)
	repo:iterator(branch, {start=datemin, stop=datemax}):map(count)
	if last == 0 then
		error("No data on this branch")
	end

	local p = pepper.gnuplot:new()
	pepper.plotutils.setup_output(p)
	if split == "none" then
		pepper.plotutils.setup_std_time(p)
		p:set_title("Activity (on " .. branch .. ")")
	else
		pepper.plotutils.setup_std_time(p, {key = "below"})
		p:set_title("Activity by " .. string.upper(split:sub(1, 1)) .. split:sub(2) .. " (on " .. branch .. ")")
	end
	p:cmd("set noytics")

	if self:getopt("tags") ~= nil then
		pepper.plotutils.add_tagmarks(p, repo, self:getopt("tags", "*"))
	end

	-- Generate graphs
	p:set_xrange_time(firstdate, lastdate)

	if split == "none" then
		local dates, values = convolution(firstdate, lastdate, activity)
		p:plot_series(dates, values, {}, "lines smooth bezier linewidth 1.5")
	else 
		-- Determine contributions (i.e. frequent entries)
		local freqs = {}
		for k,v in pairs(activity) do
			table.insert(freqs, {k, tablesize(v)})
		end
		table.sort(freqs, function (a,b) return (a[2] > b[2]) end)

		local n = 1 + tonumber(self:getopt("n", 6))
		local i = 1
		local vdates = {}
		local values = {}
		local keys = {}
		while i <= #freqs and i < n do
			vdates[i], values[i] = convolution(firstdate, lastdate, activity[freqs[i][1]])
			keys[i] = freqs[i][1]
			i = i + 1
		end

		p:plot_multi_series(vdates, values, keys, "lines smooth bezier linewidth 1.5")
	end
end
