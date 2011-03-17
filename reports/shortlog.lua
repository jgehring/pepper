--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2011 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: shortlog.lua
	Generates a summarized revision log (like "git shortlog")
--]]

-- Script meta-data
meta.title = "Shortlog"
meta.description = "Summarized revision log"
meta.options = {
	{"-bARG, --branch=ARG", "Select branch"},
	{"-s, --summary", "Print commit count summary only"}
}


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
function main()
	-- Commit message dictionary, indexed by author
	messages = {}

	-- Gather data
	local repo = pepper.report.repository()
	local branch = pepper.report.getopt("b,branch", repo:default_branch())
	repo:iterator(branch):map(callback)

	-- Sort commit dictionary by name
	local authors = {}
	for author,revisions in pairs(messages) do
		table.insert(authors, author)
	end
	table.sort(authors)

	-- Print results
	if pepper.report.getopt("s,summary") == nil then
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
			print(str .. "  " .. author)
		end
	end
end
