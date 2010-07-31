--[[
	TODO for the lua bindings:

	- Write a base class for all scripts, with methods to run callbacks on
	  on all/selected/... revisions.

	- Pass the repository backend as "Repository" to the script, so it can
	  be examined
--]]


-- Lua Functions
function Repository:show()
	print("Repository with backend =", self:type())
end

-- Example
repository:show()
