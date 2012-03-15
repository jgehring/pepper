--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: csv.lua
	Dumps commit data in a CSV format
--]]

require "pepper.datetime"


-- Describes the report
function describe(self)
	local r = {}
	r.title = "CSV"
	r.description = "Dumps commit data in CSV format"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"-cARG, --columns=ARG",
			"Comma-seperated list of columns. " ..
			"Possible values (and abbreviations) are 'id', " ..
			"'author' (a), 'added' (+), 'removed' (-), " ..
			"'delta' (d), 'total' (t), 'files' (f), " ..
			"'message' (m)"}
	}
	pepper.datetime.add_daterange_options(r)
	return r
end

-- Returns the name of the given code
function codename(code)
	if code == "id" then
		return "Commit ID"
	elseif code == "a" or code == "author" then
		return "Author"
	elseif code == "+" or code == "added" then
		return "Lines added"
	elseif code == "-" or code == "removed" then
		return "Lines removed"
	elseif code == "f" or code == "files" then
		return "Changed Files"
	elseif code == "t" or code == "total" then
		return "Total lines"
	elseif code == "d" or code == "delta" then
		return "Line delta"
	elseif code == "m" or code == "message" then
		return "Message"
	else
		error("Unkown column code: " .. code)
	end
end

-- Returns revision information given a field code
function info(r, code)
	if code == "id" then
		return r:id()
	elseif code == "a" or code == "author" then
		return r:author()
	elseif code == "+" or code == "added" then
		return r:diffstat():lines_added()
	elseif code == "-" or code == "removed" then
		return r:diffstat():lines_removed()
	elseif code == "f" or code == "files" then
		local str = ""
		for i,v in ipairs(r:diffstat():files()) do
			str = str .. v .. ":"
		end
		return str:sub(0, #str-1)
	elseif code == "t" or code == "total" then
		return loc
	elseif code == "d" or code == "delta" then
		return r:diffstat():lines_added() - r:diffstat():lines_removed()
	elseif code == "m" or code == "message" then
		local str = r:message()
		str = str:gsub('[,"]', "\\%1")
		str = str:gsub('\n', "\\n")
		str = str:gsub('\r', "\\r")
		return '"' .. str .. '"'
	else
		error("Unkown column code: " .. code)
	end
end

-- Checks if a table contains a value
function contains(t, v)
	for _,u in ipairs(t) do
		if u == v then
			return true
		end
	end
	return false
end

-- Main report function
function run(self)
	-- Global counters
	loc = 0

	local columns = pepper.utils.split(self:getopt("c,columns", "a,d,t"), ",")
	local printheader = true

	-- Gather data, but start at the beginning of the repository
	-- to get a proper LOC count if needed
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	local datemin, datemax = pepper.datetime.date_range(self)
	local itstart = datemin
	if contains(columns, "t") or contains(columns, "total") then itstart = -1 end

	repo:iterator(branch, {start=itstart, stop=datemax}):map(
		function (r)
			if printheader then
				local header = "# Timestamp, "
				for i,v in ipairs(columns) do
					header = header .. codename(v) .. ", "
				end
				print(header:sub(0, #header-2))
				printheader = false
			end

			loc = loc + r:diffstat():lines_added() - r:diffstat():lines_removed()
			if r:date() >= datemin then
				local str = tostring(r:date())
				for i,v in ipairs(columns) do
					str = str .. "," .. info(r, v)
				end
				print(str)
			end
		end
	)
end
