--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: filetypes.lua
	Visualizes the file type distribution using a histogram.
--]]

require "pepper.plotutils"


-- Describes the report
function describe(self)
	local r = {}
	r.title = "Files"
	r.description = "Histogram of the file types distribution"
	r.options = {
		{"-rARG, --revision=ARG", "Select revision (defaults to HEAD)"},
		{"-nARG", "Show the ARG most frequent file types"},
		{"-p, --pie", "Generate pie chart instead of histogram"},
	}
	pepper.plotutils.add_plot_options(r)
	return r
end

-- Maps file extensions to languages
extmap = {
	[".h"] = "C/C++",
	[".hxx"] = "C/C++",
	[".hpp"] = "C/C++",
	[".c"] = "C/C++",
	[".cc"] = "C/C++",
	[".cxx"] = "C/C++",
	[".cpp"] = "C/C++",
	[".txx"] = "C/C++",
	[".java"] = "Java",
	[".lua"] = "Lua",
	[".s"] = "Assembly",
	[".pl"] = "Perl",
	[".pm"] = "Perl",
	[".sh"] = "Shell",
	[".py"] = "Python",
	[".txt"] = "Text",
	[".po"] = "Gettext",
	[".m4"] = "Autotools",
	[".at"] = "Autotools",
	[".ac"] = "Autotools",
	[".am"] = "Autotools",
	[".htm"] = "HTML",
	[".html"] = "HTML",
	[".xhtml"] = "HTML",
	[".bat"] = "Batch",
	[".cs"] = "C#",
	[".hs"] = "Haskell",
	[".lhs"] = "Haskell",
	[".js"] = "JavaScript",
	[".php"] = "PHP",
	[".php4"] = "PHP",
	[".php5"] = "PHP",
	[".scm"] = "Scheme",
	[".ss"] = "Scheme",
	[".vb"] = "Visual Basic",
	[".vbs"] = "Visual Basic",
	[".go"] = "Go",
	[".rb"] = "Ruby",
	[".m"] = "Objective-C"
}

-- Returns the extension of a file
function extension(filename)
	local file,n = string.gsub(filename, "(.*/)(.*)", "%2")
	local ext,n = string.gsub(file, ".*(%..+)", "%1")
	if #ext == #file then
		return nil
	end
	return string.lower(ext)
end 

-- Main report function
function run(self)
	local rev = self:getopt("r, revision", "")
	local tree = self:repository():tree(rev)

	-- For now, simply count the number of files for each extension
	local count = {}
	for i,v in ipairs(tree) do
		local ext = extension(v)
		local nam = ""
		if ext ~= nil then
			nam = extmap[ext]
			if nam == nil then
				nam = string.upper(ext:sub(2))
			end
		else
			nam = "unknown"
		end
		if count[nam] == nil then
			count[nam] = 0
		end
		count[nam] = count[nam] + 1
	end

	-- Sort by number of files
	local extcount = {}
	for k,v in pairs(count) do
		table.insert(extcount, {k, v})
	end
	table.sort(extcount, function (a, b) return a[2] > b[2] end)

	-- Use descriptive titles
	local keys = {}
	local values = {}
	local n = tonumber(self:getopt("n", 6))
	local others = 0
	local unknown_idx = -1
	for i,v in pairs(extcount) do
		if i <= n then
			table.insert(keys, v[1])
			table.insert(values, v[2])
			if v[1] == "unknown" then
				unknown_idx = #keys
			end
		else
			others = others + v[2]
		end
	end
	
	-- "unkown" should go last in the list
	if unknown_idx >= 0 then
		keys[unknown_idx],keys[#keys] = keys[#keys],keys[unknown_idx]
		values[unknown_idx],values[#values] = values[#values],values[unknown_idx]
	end

	local p = pepper.gnuplot:new()

	if self:getopt("p, pie") then
		pepper.plotutils.setup_output(p, 740, 600)
	else
		pepper.plotutils.setup_output(p, 600, 300)
	end

	if #rev > 0 then
		p:set_title("File types (at " .. rev .. ")")
	else
		p:set_title("File types")
	end

	if self:getopt("p, pie") then
		if others > 0 then
			table.insert(keys, "Other")
			table.insert(values, others)
		end
		p:cmd([[
set key box
set key right outside
]])
		p:plot_pie(keys, pepper.plotutils.normalize_pie(values))
	else
		p:cmd([[
set style fill solid border -1
set style histogram cluster gap 1
set xtics nomirror
set yrange [0:]
set ylabel "Number of files"
]])
		p:plot_histogram(keys, values)
	end
end
