--[[
	Sample report
--]]


-- Lua Functions
function print_repo(r)
	print("Current repository:")
	print("Url:", r:url())
	print("Type:", r:type())
	print("Head:", assert(r:head()))
	print("Branches:")
	for i,val in ipairs(assert(r:branches())) do
		print("", val)
	end
end


loc = {}

function print_revision(r)
--	print(r:id(), r:author(), os.date("%Y-%d-%m %X", r:date()))
	s = r:diffstat()
	t = s:files()
	l = 0
	if not (next(loc) == nil) then
		l = loc[#loc]
	end
	for i,v in ipairs(t) do
		-- print("", v, s:linesAdded(v), s:linesRemoved(v)
		l = l + s:linesAdded(v) - s:linesRemoved(v)
	end
	table.insert(loc, l)
end

-- Example
--print_repo(pepper.report.repository())

-- Simple example: Map a function to all revision of a given branch
pepper.report.map_branch(print_revision, "trunk")

-- Generate graph with LOC
print("# Revision LOC")
for i,v in ipairs(loc) do
	print((i-1) .. " " .. v)
end
