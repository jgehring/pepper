--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: commits_per_month.lua
	Generates a histogram using commits per month and the amount of files touched.
	NOTE: The implementation trusts the normalizing behaviour of mktime().
--]]

require "pepper.plotutils"


-- Describes the report
function describe(self)
	local r = {}
	r.title = "Commit counts"
	r.description = "Histogramm of commit counts"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"-pARG, --period=ARG", "Show counts for the last 'Ndays', 'Nweeks', 'Nmonths' or 'Nyears'. The default is '12months'"},
		{"-rARG, --resolution=ARG", "Set histogram resolution to 'days', 'weeks', 'months', 'years' or 'auto' (the default)"}
	}
	pepper.plotutils.add_plot_options(r)
	return r
end

-- Subtracts a time period in text form from a timestamp
function subdate(t, period)
	local n = tonumber(period:match("^[%d]+")) - 1
	assert(n >= 0, string.format("Invalid time range: %q", period))
	local m = period:match("[%a]+$")

	-- Round towards zero and subtract time
	local date = os.date("*t", t)
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
		if date.year > 1971 then return "years"
		elseif date.month > 3 or date.year ~= 1970 then return "months"
		elseif date.day > 15 or date.month ~= 1 then return "weeks"
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

-- Main report function
function run(self)
	-- First, determine time range for the revision iterator, according
	-- to the --period argument
	local period = self:getopt("p,period", "12months")
	local now = os.time()
	local start = subdate(now, period)

	-- Determine time resolution
	local resolution = resolution(now - start, self:getopt("r,resolution", "auto"))
	start = timeslot(start, resolution)

	-- Gather data
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	local data = {} -- {commits, changes}
	repo:iterator(branch, {start=start}):map(
		function (r)
			local slot = timeslot(r:date(), resolution)
			if data[slot] == nil then data[slot] = {0, 0} end
			data[slot][1] = data[slot][1] + 1
			data[slot][2] = data[slot][2] + #r:diffstat():files()
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
	pepper.plotutils.setup_output(p, 800, 480)
	p:set_title(string.format("Commit Counts per %s (on %s)", resolution:sub(0, #resolution-1), branch))
	p:cmd([[
set format y "%'.0f"
set yrange [0:*]
set grid ytics
set ytics scale 0 
set key box
set key below
set xtics nomirror
set xtics rotate by -45
set style fill solid border -1
set style histogram cluster gap 1
]])
	p:plot_histogram(keys, values, {"Commits", "Changes"})
end
