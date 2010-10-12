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
	print("Head:", r:head())
	print("Branches:")
	for i,val in ipairs(r:branches()) do
		print("", val)
	end
end

-- Example
print_repo(g_repository)
