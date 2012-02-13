--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: authors.lua
	Visualizes the participation of a single author.
--]]

require "pepper.plotutils"


-- Describes the report
function describe(self)
	local r = {}
	r.name = "Participation"
	r.description = "Participation of a single author"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"-aARG, --author=ARG", "Show participation of author ARG"},
		{"-c, --changes, -l", "Count line changes instead of commit counts"},
		{"-pARG, --period=ARG", "Show counts for the last 'Ndays', 'Nweeks', 'Nmonths' or 'Nyears'. The default is '12months'"},
		{"-rARG, --resolution=ARG", "Set histogram resolution to 'days', 'weeks', 'months', 'years' or 'auto' (the default)"}
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

-- Estimates a resolution for the given time span
function resolution(d, arg)
	if arg == "d" or arg == "days" then return "days"
	elseif arg == "w" or arg == "weeks" then return "weeks"
	elseif arg == "m" or arg == "months" then return "months"
	elseif arg == "y" or arg == "years" then return "years"
	elseif arg == "a" or arg == "auto" then
		date = os.date("*t", d)
		if date.year > 1974 then return "years"
		elseif (date.month > 6 and date.year == 1971) or date.year > 1971 then return "months"
		elseif (date.day > 15 and date.month == 2) or date.month > 2 then return "weeks"
		else return "days" end
	else
		error(string.format("Unkown resolution %q", arg))
	end
end

-- Returns the correct time slot for the given resolution
function timeslot(t, resolution)
	-- Do a simple rounding towards zero, based on the given resolution
	date = os.date("*t", t)
	date.hour = 0; date.min = 0; date.sec = 0
	if resolution == "weeks" then date.day = date.day - date.wday + 1
	elseif resolution == "months" then date.day = 1
	elseif resolution == "years" then date.day = 1; date.month = 1
	end
	date.isdst = false
	return os.time(date)
end

-- Returns the next time slot for the given resolution
function nextslot(t, resolution)
	date = os.date("*t", t)
	if resolution == "days" then date.day = date.day + 1
	elseif resolution == "weeks" then date.day = date.day + 7
	elseif resolution == "months" then date.month = date.month + 1
	elseif resolution == "years" then date.year = date.year + 1
	end
	date.isdst = false
	return os.time(date)
end

-- Returns an appropriate textual description of the date
function slotkey(t, resolution)
	if resolution == "days" then return os.date("%Y-%m-%d", t)
	elseif resolution == "weeks" then return "Week " .. math.floor(1 + (os.date("*t", t).yday / 7)) .. ", " .. os.date("%Y", t)
	elseif resolution == "months" then return os.date("%b %y", t)
	elseif resolution == "years" then return os.date("%Y", t)
	end
	return tostring(t)
end

-- Tries to determine the currently used credentials of the given repo
function determine_author(repo)
	local author = nil
	if repo:type() == "git" then
		local f = io.popen("git config user.name"); -- GIT_DIR already set by pepper
		author = f:read()
		f:close()
	end
	return author
end

-- Main report function
function run(self)
	local repo = self:repository()

	-- Which author should be tracked
	local author = self:getopt("a,author")
	if author == nil then
		author = determine_author(repo)
		assert(author ~= nil, "Please specifiy an author name")
	end

	-- First, determine time range for the revision iterator, according
	-- to the --period argument
	local period = self:getopt("p,period", "12months")
	local now = os.time()
	local start = subdate(now, period)

	-- Determine time resolution
	local resolution = resolution(now - start, self:getopt("r,resolution", "auto"))
	start = timeslot(start, resolution)

	local count_changes = self:getopt("c,changes,l")

	-- Gather data
	local branch = self:getopt("b,branch", repo:default_branch())
	local data = {} -- {commits, changes}
	repo:iterator(branch, {start=start}):map(
		function (r)
			local slot = timeslot(r:date(), resolution)
			if data[slot] == nil then data[slot] = {0, 0} end

			local n = 1
			if count_changes then
				n = r:diffstat():lines_added()
			end

			if r:author() == author then
				data[slot][1] = data[slot][1] + n
			else
				data[slot][2] = data[slot][2] + n
			end
		end
	)

	-- Generate plot data by iterating over all possible time slots
	local keys = {}
	local values = {}
	local slot = start
	while slot < now do
		table.insert(keys, slotkey(slot, resolution))
		if data[slot] ~= nil then table.insert(values, data[slot])
		else table.insert(values, {0, 0}) end

		slot = timeslot(nextslot(slot, resolution))
	end

	-- Generate graph
	local p = pepper.gnuplot:new()
	pepper.plotutils.setup_output(p, 800, 200)
	p:set_title(string.format("Participation of %s per %s (on %s)", author, resolution:sub(0, #resolution-1), branch))
	p:cmd([[
set noytics
set key below
set noxtics
set style fill solid
set style histogram rowstacked
set boxwidth 0.8 relative
]])
	if count_changes then
		p:plot_histogram(keys, values, {string.format("Contributions by %s", author), "All Contributions"})
	else
		p:plot_histogram(keys, values, {string.format("Commits by %s", author), "All Commits"})
	end
end
