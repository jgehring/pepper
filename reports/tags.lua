--[[
	Shows all tags
--]]


-- Script meta-data
meta.title = "Tags"

-- Main script function
function main()
	local repo = pepper.report.repository()
	local tags = repo:tags()
	local maxlen = 0
	for i,v in ipairs(tags) do
		if #v:name() > maxlen then
			maxlen = #v:name()
		end
	end
	maxlen = maxlen + 2
	for i,v in ipairs(tags) do
		local line = v:name()
		while #line < maxlen do
			line = line .. " "
		end
		line = line .. v:id()
		print(line)
	end
end
