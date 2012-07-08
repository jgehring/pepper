--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: shortlog.lua
	Generates a summarized revision log (like "git shortlog")
--]]

require "pepper.datetime"


-- Describes the report
function describe(self)
	local r = {}
	r.title = "Shortlog"
	r.description = "Summarized revision log"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"},
		{"-s, --summary", "Print commit count summary only"},
		{"-n, --numbered", "Sort output according to number of commits"},
	}
	pepper.datetime.add_daterange_options(r)
	return r
end

-- Revision callback function
function callback(r)
	if r:author() ~= "" then
		if messages[r:author()] == nil then
			messages[r:author()] = {}
		end
		table.insert(messages[r:author()], r:message())
	end
end

-- Main report function
function run(self)
	-- Commit message dictionary, indexed by author
	messages = {}

	-- Gather data
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	local datemin, datemax = pepper.datetime.date_range(self)
	repo:iterator(branch, {start=datemin, stop=datemax}):map(callback)

	-- Sort commit dictionary
	local authors = {}
	for author,revisions in pairs(messages) do
		table.insert(authors, author)
	end
	if self:getopt("n,numbered") then
		table.sort(authors, function (a,b)
			return #messages[a] > #messages[b]
		end)
	else
		table.sort(authors)
	end

	-- Print results
	if self:getopt("s,summary") == nil then
		for i,author in ipairs(authors) do
			print(author .. " (" .. #messages[author] .. "):")
			for j,msg in ipairs(messages[author]) do
				split = string.find(msg, "\n\n")
				if split ~= nil then
					msg = string.sub(msg, 1, split)
				else
					split = string.find(msg, "\n")
					if split ~= nil then
						msg = string.sub(msg, 1, split)
					end
				end
				msg = string.gsub(msg, "\n", "    ")
				msg = string.gsub(msg, "^%s*(.-)%s*$", "%1")
				print("      " .. msg)
			end
			print()
		end
	else
		for i,author in ipairs(authors) do
			local str = tostring(#messages[author])
			while #str < 6 do
				str = " " .. str
			end
			print(str .. "\t" .. author)
		end
	end
end
