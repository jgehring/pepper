--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2011 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: branches.lua
	Shows all branches and their current heads, similar to "git branch -v".
--]]

-- Script meta-data
meta.title = "Branches"
meta.description = "Lists all repository branches"


-- Main script function
function main()
	local repo = pepper.report.repository()
	local branches = repo:branches()
	local main = repo:default_branch()
	local maxlen = 0
	for i,v in ipairs(branches) do
		if #v > maxlen then
			maxlen = #v
		end
	end
	maxlen = maxlen + 1
	for i,v in ipairs(branches) do
		local line = ""
		if v == main then
			line = "* " .. v
		else
			line = "  " .. v
		end
		j = #v
		while j < maxlen do
			line = line .. " "
			j = j + 1
		end
		line = line .. repo:head(v)
		print(line)
	end
end
