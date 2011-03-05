--- Common utility functions for plotting.

module("pepper.plotutils", package.seeall)


--- Converts from UNIX to Gnuplot epoch.
--  @param time UNIX timestamp
function convepoch(time)
	return time - 946684800
end

--- Adds x2tics for repository tags.
--  If <code>pattern</code> is not <code>nil</code>, only tags matching
--  the <code>pattern</code> will be added.
--  @param plot pepper.plot obejct
--  @param repo pepper.repository object
--  @param pattern Optional pattern for filtering tags
function add_tagmarks(plot, repo, pattern)
	-- Fetch tags and generate tic data
	local tags = repo:tags()
	if #tags == 0 then
		return
	end
	local x2tics = "("
	for k,v in ipairs(tags) do
		if pattern == nil or v:name():find(pattern) ~= nil then
			x2tics = x2tics .. "\"" .. v:name() .. "\" " .. convepoch(repo:revision(v:id()):date()) .. ","
		end
	end
	if #x2tics == 1 then
		return
	end
	x2tics = x2tics:sub(0, #x2tics-1) .. ")"
	plot:cmd([[
set x2data time
set format x2 "%s"
set x2tics scale 0
set x2tics border rotate by 60
set x2tics font "Helvetica,8"
]])
	plot:cmd("set x2tics " .. x2tics)
end

--- Performs a standard plot setup for time data.
--  @param plot pepper.plot object
--  @param options Optional dictionary with additional options
function setup_std_time(plot, options)
	plot:cmd([[
set xdata time
set timefmt "%s"
set format x "%b %y"
set format y "%'.0f"
set yrange [0:*]
set xtics nomirror
set xtics rotate by -45
set rmargin 8
set grid ytics
set grid x2tics
]])
	if options ~= nil then
		if options.key ~= nil then
			plot:cmd("set key box; set key " .. options.key)
		end
	end
end
