--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2014 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: tags.lua
	Shows all tags and their corresponding revisions.
	NOTE: This report is mainly used for testing purposes
--]]


-- Describes the report
function describe(self)
	local r = {}
	r.name = "Tags"
	r.description = "Lists all tags"
	return r
end

-- Main script function
function main()
	local repo = pepper.current_report():repository()
	local tags = repo:tags()
	local maxlen = 0
	for i,v in ipairs(tags) do
		if #v:name() > maxlen then
			maxlen = #v:name()
		end
	end
	maxlen = maxlen + 2
	for i,v in ipairs(tags) do
		local line = v:name()
		while #line < maxlen do
			line = line .. " "
		end
		line = line .. v:id()
		print(line)
	end
end
