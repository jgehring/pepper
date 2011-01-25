--[[
	Generates a histogram visualizing type file type distribution
--]]


-- Script meta-data
meta.title = "File types histogram"
meta.description = "A histogram of the file types distribution"
meta.options = {{"-rARG, --revision=ARG", "Select revision (defaults to HEAD)"},
                {"-nARG", "Show the ARG most frequent file types"},
                {"-oARG, --output=ARG", "Select output file (defaults to stdout)"},
                {"-tARG, --type=ARG", "Explicitly set image type"},
                {"-sW[xH], --size=W[xH]", "Set image size to width W and height H"}}

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
	[".s"] = "Assembler",
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
	[".go"] = "Go"
}

-- Sets up the plot according to the command line arguments
function setup_plot(id)
	local p = pepper.gnuplot:new()
	if #id > 0 then
		p:set_title("File types (at " .. id .. ")")
	else
		p:set_title("File types")
	end

	local file = pepper.report.getopt("o, output", "")
	local size = pepper.utils.split(pepper.report.getopt("s, size", "600"), "x")
	local terminal = pepper.report.getopt("t, type")
	local width = tonumber(size[1])
	local height = width * 0.5
	if (#size > 1) then
		height = tonumber(size[2])
	end

	if terminal ~= nil then
		p:set_output(file, width, height, terminal)
	else
		p:set_output(file, width, height)
	end
	return p
end

-- Returns the extension of a file
function extension(filename)
	local file,n = string.gsub(filename, "(.*/)(.*)", "%2")
	local ext,n = string.gsub(file, ".*(%..+)", "%1")
	if #ext == #file then
		return nil
	end
	return string.lower(ext)
end 

-- Compares two {name, value} pairs
function countcmp(a, b)
	if a[1] == "unknown" then
		return false
	end
	if b[1] == "unknown" then
		return true
	end
	return (a[2] > b[2])
end

-- Main report function
function main()
	local rev = pepper.report.getopt("r, revision", "")
	local tree = pepper.report.repository():tree(rev)

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
	table.sort(extcount, countcmp)

	-- Use descriptive titles
	local keys = {}
	local values = {}
	local n = 1 + tonumber(pepper.report.getopt("n", 6))
	for i,v in pairs(extcount) do
		if i > n then
			break
		end
		if v[1] ~= "unknown" then
			table.insert(keys, v[1])
			table.insert(values, v[2])
		end
	end
	
	if count["unknown"] ~= nil then
		table.insert(keys, "unknown")
		table.insert(values, count["unknown"])
	end

	local p = setup_plot(rev)

	p:cmd([[
set style fill solid border -1
set style histogram cluster gap 1
set xtics nomirror
set ylabel "Number of files"
]])
	p:plot_histogram(keys, values)
end
