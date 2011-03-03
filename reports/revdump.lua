--[[
	Dumps a revision
--]]


-- Script meta-data
meta.title = "Revision dump"
meta.options = {{"-bARG, --branch=ARG", "Select branch"},
                {"-rARG, --revision=ARG", "Select revision"}}

-- Revision dump function
function revdump(r)
	print("-- Revision")
	print(r:id())
	print("-- Date")
	print(r:date())
	print("-- Author")
	print(r:author())
	print("-- Message ")
	print(r:message())
	print("-- Diffstat")
	local d = r:diffstat()
	for i,f in ipairs(d:files()) do
		print(f .. " +" .. d:lines_added(f) .. " -" .. d:lines_removed(f) .. " +" .. d:bytes_added(f) .. " -" .. d:lines_removed(f))
	end
	print("==================================================================")
end

-- Main script function
function main()
	local repo = pepper.report.repository()
	local rev = pepper.report.getopt("r,revision")
	if rev ~= nil then
		revdump(repo:revision(rev))
	else
		local branch = pepper.report.getopt("b,branch", repo:default_branch())
		repo:iterator(branch):map(revdump)
	end
end
