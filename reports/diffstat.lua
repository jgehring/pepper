--[[
    pepper - SCM statistics report generator
    Copyright (C) 2010-2012 Jonas Gehring

    Released under the GNU General Public License, version 3.
    Please see the COPYING file in the source distribution for license
    terms and conditions, or see http://www.gnu.org/licenses/.

    file: diffstat.lua
	Pretty-prints a diffstat for a specified revision.
	The histogram rendering is inspired by the git implementation.
--]]


-- Describes the report
function describe(self)
	local r = {}
	r.title = "Diffstat"
	r.description = "Print diffstat of given revision"
	r.options = {{"-rARG, --revision=ARG", "Revision ID"}}
	return r
end

-- Graph scaling
function scale(i, width, max)
	if max < 2 then
		return i
	end
	return math.floor(((i - 1) * (width - 1) + max - 1) / (max - 1))
end

-- "Render" and return a graph
function graph(c, i, width, max)
	local n = i
	if max >= 2 then
		n = math.floor(((i - 1) * (width - 1) + max - 1) / (max - 1))
	end

	local str = ""
	local t = 0
	while t < n do
		str = str .. c
		t = t + 1
	end
	return str
end

-- File name scaling
function scale_name(name, width)
	local str = name
	if #str > width then
		local temp = str:sub(-(width - 3))
		local spos = temp:find("/")
		if spos ~= nil then
			temp = temp:sub(spos)
		end
		str = "..." .. temp
	end

	local t = #str
	while t < width do
		str = str .. " "
		t = t + 1
	end
	return str
end

-- Main report function
function run(self)
	local id = self:getopt("r,revision")
	assert(id ~= nil, "Please specify a revision ID")

	local stat = self:repository():revision(id):diffstat()

	-- Determine maximum filename length and amount of change
	local width = 80
	local max_len = 0
	local max_change = 0
	for k,v in ipairs(stat:files()) do
		if max_len < #v then
			max_len = #v
		end
		local change = (stat:lines_added(v) + stat:lines_removed(v))
		if max_change < change then
			max_change = change
		end
	end

	-- Calculate name and graph widths
	local space = 10
	local min_graph_width = 15
	local name_width = max_len
	local graph_width = max_change
	if name_width + space + graph_width ~= 80 then
		graph_width = 80 - (name_width + space)
	end
	if graph_width < min_graph_width then
		name_width = (name_width + graph_width - min_graph_width)
		graph_width = min_graph_width
	end
	if graph_width > max_change then
		graph_width = max_change
	end

	-- Print histogram
	local line = ""
	for k,v in ipairs(stat:files()) do
		line = scale_name(v, name_width)
		line = line .. " | " .. string.format("%4d ", (stat:lines_added(v) + stat:lines_removed(v)))
		line = line .. graph("+", stat:lines_added(v), graph_width, max_change)
		line = line .. graph("-", stat:lines_removed(v), graph_width, max_change)
		print(" " .. line)
	end

	-- Print summary
	print(" " .. #stat:files() .. " files changed, " .. stat:lines_added() .. " insertions(+), " .. stat:lines_removed() .. " deletions(-)")
end
