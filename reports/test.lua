--[[
	TODO for the lua bindings:

	- Write a base class for all scripts, with methods to run callbacks on
	  on all/selected/... revisions.
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

function print_revision(r)
	print(r:id(), r:date())
	s = r:diffstat()
	t = s:files()
	for i,v in ipairs(t) do print("", v, s:added(v), s:removed(v)) end
end

-- Example
print_repo(g_repository)

-- Simple example: Print all revision of a given branch
--report.map_branch(print_revision, g_repository, "trunk")
report.map_branch(print_revision, g_repository, "trunk")
