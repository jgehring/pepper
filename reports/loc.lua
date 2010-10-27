--[[
	Generate LOC and BOC graphs
--]]


require("luaplot.dotlineplot")
require("luaplot.pnghandler")


loc = {}
boc = {}
dates = {}
lines = 0
bytes = 0
function count(r)
	s = r:diffstat()
	t = s:files()
	for i,v in ipairs(t) do
		lines = lines + s:linesAdded(v) - s:linesRemoved(v)
		bytes = bytes + s:bytesAdded(v) - s:bytesRemoved(v)
	end
	-- TODO: Use dictionaries instead of arrays
	table.insert(loc, lines)
	table.insert(boc, bytes)
	table.insert(dates, r:date())
end

-- Gather data
pepper.report.map_branch(count, "trunk")

-- Generate graphs
local p = pepper.plot:new()
p:set_title("Lines of Code (trunk)")
p:set_output("loc.svg")
p:plotty(dates, loc)

local p2 = pepper.plot:new()
p2:set_title("Bytes of Code (trunk)")
p2:set_output("boc.svg")
p2:plotty(dates, boc)
