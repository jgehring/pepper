--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: commit_volume.lua
	Stacked bar plots for commit volumes per author.
	Idea from Ohloh, e.g. http://www.ohloh.net/p/ohcount/analyses/latest
--]]

require "pepper.plotutils"


-- Describes the report
function describe(self)
	local r = {}
	r.title = "Commit volume"
	r.description = "Stacked bar plots for commit volumes"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"--slots=ARG", "Comma-separated list of time slots ('all','Ndays, 'Nweeks, 'Nmonths' or 'Nyears')"},
		{"-c, --changes, -l", "Count line changes instead of commit counts"},
		{"-nARG", "Show the ARG busiest authors"},
	}
	pepper.plotutils.add_plot_options(r)
	return r
end

-- Subtracts a time period in text form from a timestamp
function subdate(t, period)
	n = tonumber(period:match("^[%d]+")) - 1
	assert(n >= 0, string.format("Invalid time range: %q", period))
	m = period:match("[%a]+$")

	-- Round towards zero and subtract time
	date = os.date("*t", t)
	date.hour = 0
	date.min = 0
	date.sec = 0
	if m == "d" or m == "days" then
		date.day = date.day - n
	elseif m == "w" or m == "weeks" then
		date.day = date.day - date.wday + 1
		date.day = date.day - 7*n
	elseif m == "m" or m == "months" then
		date.day = 1
		date.month = date.month - n
	elseif m == "y" or m == "years" then
		date.day = 1
		date.month = 1
		date.year = date.year - n
	else
		error(string.format("Unkown time unit %q", m))
	end

	return os.time(date)
end

-- Parses timeslot information
-- Timeslot format: { caption, min_time }
function timeslots(str)
	local slots = {}
	local now = os.time()
	for _,v in ipairs(pepper.utils.split(str, ",")) do
		if not v:match("^[%d]+") then v = "1" .. v  end
		local n = tonumber(v:match("^[%d]+")) - 1
		assert(n >= 0, string.format("Invalid time range: %q", v))
		local m = v:match("[%a]+$")

		local date = os.date("*t", t)
		date.hour = 0
		date.min = 0
		date.sec = 0

		if m == "a" or m == "all" then
			table.insert(slots, {"All Time", -1})
		elseif m == "d" or m == "days" then
			date.day = date.day - n
			if n == 0 then table.insert(slots, {"Today", os.time(date)})
			else table.insert(slots, {"Past " .. n+1 .. " days", os.time(date)}) end
		elseif m == "w" or m == "weeks" then
			date.day = date.day - date.wday + 1
			date.day = date.day - 7*n
			if n == 0 then table.insert(slots, {"This week", os.time(date)})
			else table.insert(slots, {"Past " .. n+1 .. " weeks", os.time(date)}) end
		elseif m == "m" then
			date.day = 1
			date.month = date.month - n
			if n == 0 then table.insert(slots, {"This month", os.time(date)})
			else table.insert(slots, {"Past " .. n+1 .. " months", os.time(date)}) end
		elseif m == "y" or m == "years" then
			date.day = 1
			date.month = 1
			date.year = date.year - n
			if n == 0 then table.insert(slots, {"This year", os.time(date)})
			else table.insert(slots, {"Past " .. n+1 .. " years", os.time(date)}) end
		end
	end
	return slots
end

-- Main report function
function run(self)
	-- Determine timeslots
	local slots = timeslots(self:getopt("slots", "all,12m,m"))
	local data = {}
	local total = {}
	local authors = {}
	local author_names = {}
	for i,v in ipairs(slots) do
		data[v] = {}
		total[v] = 0
	end
	local count_changes = self:getopt("c,changes,l")

	-- Determine minimum date
	local datemin = -1
	for _,v in ipairs(slots) do
		if v[2] < datemin then datemin = v[2] end
	end

	-- Gather data
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	repo:iterator(branch, {start=datemin}):map(
		function (r)
			local n = 1
			if count_changes ~= nil then
				n = r:diffstat():lines_added() + r:diffstat():lines_removed()
			end

			for i,v in ipairs(slots) do
				if r:date() >= v[2] then
					if data[v][r:author()] == nil then data[v][r:author()] = n
					else data[v][r:author()] = data[v][r:author()] + n end
					total[v] = total[v] + n

					if authors[r:author()] == nil then
						authors[r:author()] = n
					else
						authors[r:author()] = authors[r:author()] + n
					end
				end
			end
		end
	)

	-- Sort authors by contribution
	local author_names = {}
	for k,v in pairs(authors) do table.insert(author_names, k) end
	table.sort(author_names, function (a,b) return authors[a] > authors[b] end)
	local num_authors = 1 + tonumber(self:getopt("n", 6))
	if num_authors <= #author_names then
		author_names[num_authors] = nil
	end

	local keys = {}
	local values = {}
	local sums = {}
	local fill_others = false
	for i,v in ipairs(slots) do
		table.insert(keys, v[1])
		local vals = {}
		local sum = 0.0
		for j,author in ipairs(author_names) do
			if data[v][author] then table.insert(vals, 100.0 * data[v][author] / total[v])
			else table.insert(vals, 0.0) end
			sum = sum + vals[#vals]
		end
		table.insert(values, vals)

		table.insert(sums, sum)
		if sum < 100 then fill_others = true end
	end

	if fill_others then
		table.insert(author_names, num_authors, "Others")
		for i,v in ipairs(values) do
			table.insert(v, 100.0 - sums[i])
		end
	end

	local p = pepper.gnuplot:new()
	pepper.plotutils.setup_output(p, 800, 480)
	p:cmd([[
set style histogram rowstacked
set style fill solid border -1
set style fill solid border -1
set style data histogram
set style histogram rowstacked
set boxwidth 0.75
set key box
set key right outside
set yrange [0:100]
]])
	if count_changes then
		p:cmd("set ylabel \"% of Contribution\"")
		p:set_title("Contribution Volume by Author (on " .. branch .. ")")
	else
		p:cmd("set ylabel \"% of Commits\"")
		p:set_title("Commit Volume by Author (on " .. branch .. ")")
	end
	p:plot_histogram(keys, values, author_names)
end
