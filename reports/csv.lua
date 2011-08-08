--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2011 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: csv.lua
	Dumps commits in a CSV format
--]]


-- Describes the report
function describe(self)
	local r = {}
	r.title = "CSV"
	r.description = "Dumps commits in CSV format"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"-cARG, --columns=ARG",
			"Comma-seperated list of columns. " ..
			"Possible values (and abbreviations) are 'id', " ..
			"'author' (a), 'added' (+), 'removed' (-), " ..
			"'delta' (d), 'total' (t), 'files' (f), " ..
			"'message' (m)"},
		{"--datemin=ARG", "Start date (format is YYYY-MM-DD)"},
		{"--datemax=ARG", "End date (format is YYYY-MM-DD)"}
	}
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
	local delta = r:diffstat():lines_added() - r:diffstat():lines_removed()
	loc = loc + delta

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
		return delta
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

-- Main report function
function run(self)
	-- Global counters
	loc = 0

	-- Parse date range
	local datemin = self:getopt("datemin")
	if datemin ~= nil then datemin = pepper.utils.strptime(datemin, "%Y-%m-%d")
	else datemin = -1 end
	local datemax = self:getopt("datemax")
	if datemax ~= nil then datemax = pepper.utils.strptime(datemax, "%Y-%m-%d")
	else datemax = -1 end

	local columns = pepper.utils.split(self:getopt("c,columns", "a,d,t"), ",")
	local printheader = true

	-- Gather data
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	repo:iterator(branch, {start=datemin, stop=datemax}):map(
		function (r)
			if printheader then
				local header = "# Timestamp, "
				for i,v in ipairs(columns) do
					header = header .. codename(v) .. ", "
				end
				print(header:sub(0, #header-2))
				printheader = false
			end

			local str = tostring(r:date())
			for i,v in ipairs(columns) do
				str = str .. "," .. info(r, v)
			end
			print(str)
		end
	)
end
