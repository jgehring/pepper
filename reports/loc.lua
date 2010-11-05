--[[
-- Generate LOC and BOC graphs
--]]


-- Print usage information
function print_help()
	print("Report options for 'shortlog':")
	pepper.report.print_option("-b, --branch", "Select branch")
end


-- Revision callback function
function count(r)
	s = r:diffstat()
	t = s:files()
	for i,v in ipairs(t) do
		lines = lines + s:linesAdded(v) - s:linesRemoved(v)
		bytes = bytes + s:bytesAdded(v) - s:bytesRemoved(v)
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
	branch = pepper.report.option("-b,--branch", "trunk")
	pepper.report.map_branch(count, branch)

	-- Generate graphs
	local p = pepper.plot:new()
	p:set_title("Lines of Code (trunk)")
	p:set_output("loc.svg")
	p:plotty(dates, loc)

	local p2 = pepper.plot:new()
	p2:set_title("Bytes of Code (trunk)")
	p2:set_output("boc.svg")
	p2:plotty(dates, boc)
end
