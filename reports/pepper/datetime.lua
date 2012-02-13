--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file pepper/datetime.lua
	Common utility functions for date and time handling
--]]

--- Common utility functions for date and time handling.
--  Please note that this is a Lua module. If you want to use it, add
--  <pre>require "pepper.datetime"</pre> to your script.

module("pepper.datetime", package.seeall)


--- Adds command-line options for start and end date to <code>meta.options</code>.
--  @param r If not null, options will be added to this table
function add_daterange_options(r)
	if r then
		if not r.options then r.options = {} end
		table.insert(r.options, {"--datemin=ARG", "Start date (format is YYYY-MM-DD)"})
		table.insert(r.options, {"--datemax=ARG", "End date (format is YYYY-MM-DD)"})
	else
		if meta.options == nil then meta.options = {} end
		table.insert(meta.options, {"--datemin=ARG", "Start date (format is YYYY-MM-DD)"})
		table.insert(meta.options, {"--datemax=ARG", "End date (format is YYYY-MM-DD)"})
	end
end

--- Parses the date range from the report's command-line options.
--  If an option isn't present, -1 is returned in its place.
--  @param report The report. If \p null, pepper.current_report() will be used.
--  @returns start, end as seconds
function date_range(report)
	if report == nil then
		report = pepper.current_report()
	end
	local datemin = report:getopt("datemin")
	if datemin ~= nil then datemin = pepper.utils.strptime(datemin, "%Y-%m-%d")
	else datemin = -1 end
	local datemax = report:getopt("datemax")
	if datemax ~= nil then datemax = pepper.utils.strptime(datemax, "%Y-%m-%d")
	else datemax = -1 end
	return datemin, datemax
end

--- Returns a human-friendly description of a time range.
--  This is a port of the Rails funtion
--  <a href="http://apidock.com/rails/ActionView/Helpers/DateHelper/distance_of_time_in_words">distance_of_time_in_words</a>.
--  @param secs Time range in seconds, e.g. generated using <a href="http://www.lua.org/manual/5.1/manual.html#pdf-os.difftime">os.difftime()</a>.
function humanrange(secs)
	local mins = math.floor(secs / 60 + 0.5)
	if mins < 2 then
		if secs < 5 then return "less than 5 seconds"
		elseif secs < 10 then return "less than 10 seconds"
		elseif secs < 20 then return "less than 20 seconds"
		elseif secs < 40 then return "half a minute"
		elseif secs < 60 then return "less than a minute"
		else return "1 minute"
		end
	elseif mins < 45 then return tostring(mins) .. " minutes"
	elseif mins < 90 then return "about 1 hour"
	elseif mins < 1440 then return "about " .. math.floor(mins / 60) .. " hours"
	elseif mins < 2530 then return "1 day"
	elseif mins < 43200 then return math.floor(mins / 1440) .. " days"
	elseif mins < 86400 then return "about 1 month"
	elseif mins < 525600 then return math.floor(mins / 43200) .. " months"
	else
		local years = math.floor(mins / 525600)
		local leap_offset = math.floor(years / 4) * 1440
		local remainder = ((mins - leap_offset) % 525600)

		local suffix = "s"
		if years == 1 then suffix = "" end

		if remainder < 131400 then return "about " .. years .. " year" .. suffix
		elseif remainder < 394200 then return "over " .. years .. " year " ..suffix
		else return "over " .. years .. " years"
		end
	end
end
