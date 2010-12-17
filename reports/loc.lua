--[[
-- Generate LOC and BOC graphs
--]]


-- Script meta-data
meta.name = "LOC"
meta.options = {{"-b, --branch", "Select branch"},
                {"-t, --type", "Select image type"}}

-- Revision callback function
function count(r)
	s = r:diffstat()
	t = s:files()
	for i,v in ipairs(t) do
		lines = lines + s:lines_added(v) - s:lines_removed(v)
		bytes = bytes + s:bytes_added(v) - s:bytes_removed(v)
	end
	if r:date() == 0 then
		return
	end
	-- TODO: Use dictionaries instead of arrays
	table.insert(loc, lines)
	table.insert(boc, bytes)
	table.insert(dates, r:date())
--	print(bytes, lines, r:date(), r:id())
end

-- Main report function
function main()
	loc = {}
	boc = {}
	dates = {}
	lines = 0
	bytes = 0

	-- Gather data
	branch = pepper.report.getopt("-b,--branch", pepper.report.repository():main_branch())
	pepper.report.walk_branch(count, branch)

	-- Generate graphs
	local imgtype = pepper.report.getopt("-t,--type", "svg")
	local p = pepper.plot:new()
	p:set_title("Lines of Code (" .. branch .. ")")
	p:set_output("loc." .. imgtype)
	p:cmd("set terminal " .. imgtype .. " size 600,480")
	p:plotty(dates, loc)

	local p2 = pepper.plot:new()
	p2:set_title("Bytes of Code (" .. branch .. ")")
	p2:set_output("boc." .. imgtype)
	p2:cmd("set terminal " .. imgtype .. " size 600,480")
	p2:plotty(dates, boc)
end
