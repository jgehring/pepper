--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file pepper/plotutils.lua
	Common utility functions for plotting
--]]

-- Check for required modules
assert(pepper.gnuplot ~= nil, "pepper built without Gnuplot support")

--- Common utility functions for plotting.
--  Please note that this is a Lua module. If you want to use it, add
--  <pre>require "pepper.plotutils"</pre> to your script. The functions
--  in this module are provided to facilate common plotting tasks and
--  to remove duplicate code for the built-in reports.

module("pepper.plotutils", package.seeall)


--- Adds common options for graphical reports to the given table.
--  The following options will be added:
--  <table>
--  <tr><th>Option</th><th>Description</th></tr>
--  <tr><td>-oARG, --output=ARG</td><td>Select output file</td></tr>
--  <tr><td>-tARG, --type=ARG</td><td>Explicitly set image type</td></tr>
--  <tr><td>-sW[xH], --size=W[xH]</td><td>Set image size to width W and height H</td></tr>
--  </table>
--  @param r If not null, options will be added to this table. Else, the global meta.options table will be used.
function add_plot_options(r)
	if r then
		if not r.options then r.options = {} end
		table.insert(r.options, {"-oARG, --output=ARG", "Select output file"})
		table.insert(r.options, {"-tARG, --type=ARG", "Explicitly set image type"})
		table.insert(r.options, {"-sW[xH], --size=W[xH]", "Set image size to width W and height H"})
	else
		if meta.options == nil then meta.options = {} end
		table.insert(meta.options, {"-oARG, --output=ARG", "Select output file"})
		table.insert(meta.options, {"-tARG, --type=ARG", "Explicitly set image type"})
		table.insert(meta.options, {"-sW[xH], --size=W[xH]", "Set image size to width W and height H"})
	end
end

--- Converts from UNIX to Gnuplot epoch.
--  @param time UNIX timestamp
function convepoch(time)
	return time - 946684800
end

--- Sets the plot output size, type and filename from command-line options.
--  @param plot pepper.plot object
--  @param width Optional width, 640 by default
--  @param height Optional height, 480 by default
function setup_output(plot, width, height)
	if width == nil then width = 640 end
	if height == nil then height = 480 end
	local ratio = height / width

	local self = pepper.current_report()
	local file = self:getopt("o, output", "")
	local size = pepper.utils.split(self:getopt("s, size", "" .. width .. "x" .. height), "x")
	local terminal = self:getopt("t, type")
	width = tonumber(size[1])
	height = width * ratio
	if (#size > 1) then
		height = tonumber(size[2])
	end

	if terminal ~= nil then
		plot:set_output(file, width, height, terminal)
	else
		plot:set_output(file, width, height)
	end
end

--- Performs a standard plot setup for time data.
--  Basically, this evaluates to the following GNUPlot commands (without comments):
--  <pre> set xdata time           # X values of data are time values<br>
--  set timefmt "%s"         # Time values are given as UNIX timestamps<br>
--  set yrange [0:*]         # Start Y axis at 0<br>
--  set xtics nomirror       # Don't mirror X axis tics<br>
--  set xtics rotate by -45  # Rotate X axis labels<br>
--  set rmargin 8            # Make sure there's enough space for the rotated labels<br>
--  set grid ytics           # Show grid lines for the Y axis tics<br></pre>
--  The <code>options</code> parameter can be used to customize the
--  plot. The following keys are supported:
--  <table>
--  <tr><th>Key</th><th>Description</th><th>Default value</th></tr>
--  <tr><td><code>key</code></td><td>Key position</td><td>No key</td></tr>
--  <tr><td><code>xformat</code></td><td>X axis labels format</td><td><code>"%b %y"</code></td></tr>
--  <tr><td><code>yformat</code></td><td>Y axis labels format</td><td><code>"%'.0f"</code></td></tr>
--  </table><br>
--  @param plot pepper.plot object
--  @param options Optional dictionary with additional options
function setup_std_time(plot, options)
	plot:cmd([[
set xdata time
set timefmt "%s"
set yrange [0:*]
set xtics nomirror
set xtics rotate by -45
set rmargin 8
set grid ytics
]])
	if options == nil then
		options = {}
	end
	if options.key ~= nil then plot:cmd("set key box; set key " .. options.key) end
	if options.xformat == nil then options.xformat = "%b %y" end
	plot:cmd("set format x \"" .. options.xformat .. "\"")
	if options.yformat == nil then options.yformat = "%'.0f" end
	plot:cmd("set format y \"" .. options.yformat .. "\"")
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
set grid x2tics
]])
	plot:cmd("set x2tics " .. x2tics)
end

--- Normalizes the given values for using the in a pie chart.
--  @param values A table of values
--  @returns A table with normalized values that sum up to one
function normalize_pie(values)
	local sum = 0
	for i,v in ipairs(values) do
		sum = sum + v
	end
	if sum == 0 then
		return values
	end

	nvalues = {}
	for i,v in ipairs(values) do
		table.insert(nvalues, v / sum)
	end
	return nvalues
end
