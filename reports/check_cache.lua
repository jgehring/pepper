--[[
	Runs a cache check
--]]

-- Script meta-data
meta.name = "Cache check"
meta.description = "Checks the revision cache"
meta.options = {}

-- Main script function
function main()
	pepper.internal.check_cache(pepper.report.repository())
end
