--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: data.lua
	Dumps commit data
--]]

require "pepper.datetime"


-- Describes the report
function describe(self)
	local r = {}
	r.title = "Data dump"
	r.description = "Dumps commit data in various formats"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"-fARG, --format=ARG", "Select format (CSV, JSON)"},
		{"-cARG, --columns=ARG",
			"Comma-seperated list of columns. Possible values " ..
			"(and abbreviations) are 'id', 'author' (a), " ..
			"'added' (+), 'removed' (-), 'delta' (d), 'total' (t), " ..
			"'files' (f), 'message' (m)." }
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
		return r:diffstat():files()
	elseif code == "t" or code == "total" then
		return loc
	elseif code == "d" or code == "delta" then
		return r:diffstat():lines_added() - r:diffstat():lines_removed()
	elseif code == "m" or code == "message" then
		return r:message()
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

-- Escape and quotes a string
function mytostring(v)
	return tostring(v):gsub("[\\\"]", "\\%1"):gsub('\n', "\\n"):gsub("\t", "\\t"):gsub("\r", "\\r")
end

-- Sets up the data renderer
function setup_renderer(format)
	local r = {}
	if format == "csv" then
		r.head = function (columns) 
			local header = "# Timestamp, "
			for i,v in ipairs(columns) do
				header = header .. codename(v) .. ", "
			end
			print(header:sub(0, #header-2))
		end
		r.tail = function () end
		r.row = function (data, first)
			local s = ""
			for _,v in ipairs(data) do
				if type(v) == "table" then v = table.concat(v, ":") end
				if type(v) == "string" then v = '"' .. mytostring(v):gsub('[,]', "\\%1") .. '"' end
				s = s .. v .. ","
			end
			print(s:sub(0, #s-1))
		end
	elseif format == "json" then
		r.head = function (columns) r.bufs = {} end
		r.tail = function ()  print("[" .. table.concat(r.bufs, ",") .. "]") end
		r.row = function (data, first)
			local s = "["
			local f = true
			for _,v in ipairs(data) do
				if not f then  s = s .. "," end
				if type(v) == "table" then
					local str = "["
					for _,f in ipairs(v) do str = str .. '"' .. mytostring(f) .. "\"," end
					v = str:sub(0, #str-1) .. "]"
				elseif type(v) == "string" then v = '"' .. mytostring(v) .. '"' end
				s =  s .. v
				f = false
			end
			table.insert(r.bufs, s .. "]")
		end
	else
		return nil
	end
	return r
end

-- Main report function
function run(self)
	-- Global counters
	loc = 0

	local columns = pepper.utils.split(self:getopt("c,columns", "a,d,t"), ",")

	-- Gather data, but start at the beginning of the repository
	-- to get a proper LOC count if needed
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	local datemin, datemax = pepper.datetime.date_range(self)
	local itstart = datemin
	if contains(columns, "t") or contains(columns, "total") then itstart = -1 end

	local format = self:getopt("f,format", "csv"):lower()
	local renderer = setup_renderer(format)
	if not renderer then
		error("Unknown format: " .. format)
	end

	renderer.head(columns)
	local first = true
	repo:iterator(branch, {start=itstart, stop=datemax}):map(
		function (r)
			loc = loc + r:diffstat():lines_added() - r:diffstat():lines_removed()

			if r:date() >= datemin then
				local data = {}
				table.insert(data, r:date())
				for i,v in ipairs(columns) do
					table.insert(data, info(r, v))
				end

				renderer.row(data, first)
				first = false
			end
		end
	)
	renderer.tail()
end
