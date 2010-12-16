--[[
-- Generate Commits per month graph
--]]


-- Script meta-data
meta.name = "Commits per Month"
meta.options = {{"-b, --branch", "Select branch"}}

-- Revision callback function
function callback(r)
	local date = os.date("*t", r:date())
	-- Ignore commits older than 1 year
	if (now["year"] - date["year"] > 1) or (now["year"] - date["year"] == 1 and now["month"] <= date["month"]) then
		return
	end

	if commits[date["month"]] == nil then
		commits[date["month"]] = 0
		changes[date["month"]] = 0
	end
	commits[date["month"]] = commits[date["month"]] + 1
	changes[date["month"]] = changes[date["month"]] + #r:diffstat():files()
end

-- Main report function
function main()
	now = os.date("*t", os.time())
	commits = {}
	changes = {}

	-- Gather data
	branch = pepper.report.getopt("-b,--branch", pepper.report.repository():main_branch())
	-- TODO: Restrict to last twelve months
	pepper.report.walk_branch(callback, branch)

	-- Generate a data file
	local names = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"}
	local month = (now["month"] % 12) + 1
	local file, filename = pepper.utils.mkstemp()
	while true do
		ncommits = 0
		nchanges = 0
		if commits[month] ~= nil then
			ncommits = commits[month]
			nchanges = changes[month]
		end
		file:write(month .. " " .. names[month] .. " " .. ncommits .. " " .. nchanges .. "\n")
		if month == now["month"] then
			break
		end
		month = (month % 12) + 1
	end

	-- Generate graphs
	local plot = pepper.plot:new()
	plot:set_title("Commits per Month (on " .. branch .. ")")
	plot:set_output("cpm.svg")
	plot:cmd([[
set terminal svg size 800,400
set format y "%.0f"
set yrange [0:*]
set grid ytics
set ytics scale 0 
set key box
set key below
set boxwidth 0.4
set xtics ("January" 1, "March" 3, "May" 5, "July" 7, "September" 9, "November" 11) scale 0
]])
	local cmd = "plot "
	cmd = cmd .. "\"" .. filename .. "\" using 1:3 with boxes fs solid title \"Commits\", "
	cmd = cmd .. "\"" .. filename .. "\" using ($1+.4):4 with boxes fs solid title \"Changes\";"

	file:close()
	plot:cmd(cmd)
	plot:flush()
	pepper.utils.unlink(filename)
end
