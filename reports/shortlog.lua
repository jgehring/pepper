--[[
-- Generate a shortlog (like "git shortlog")
--]]


-- Print usage information
function print_help()
	print("Report options for 'shortlog':")
	pepper.report.print_option("-b, --branch", "Select branch")
end


-- Revision callback function
function callback(r)
	if r:author() ~= "" then
		if messages[r:author()] == nil then
			messages[r:author()] = {}
		end
		table.insert(messages[r:author()], r:message())
	end
end

-- Main report function
function main()
	-- Commit message dictionary, indexed by author
	messages = {}

	-- Gather data
	branch = pepper.report.option("-b,--branch", "trunk")
	pepper.report.map_branch(callback, branch)

	-- Sort commit dictionary by name
	local authors = {}
	for author,revisions in pairs(messages) do
		table.insert(authors, author)
	end
	table.sort(authors)

	-- Print results
	for i,author in ipairs(authors) do
		print(author .. " (" .. #messages[author] .. "):")
		for j,msg in ipairs(messages[author]) do
			split = string.find(msg, "\n\n")
			if split ~= nil then
				msg = string.sub(msg, 1, split)
			end
			msg = string.gsub(msg, "\n", "    ")
			msg = string.gsub(msg, "^%s*(.-)%s*$", "%1")
			print("      " .. msg)
		end
		print()
	end
end
