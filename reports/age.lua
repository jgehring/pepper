--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: age.lua
	Prints the age of a repository or branch
--]]

require "pepper.datetime"


-- Describes the report
function describe(self)
	local r = {}
	r.name = "Age"
	r.description = "Repository age information"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"}
	}
	return r
end

-- Returns a counting string for the given number (e.g. 1 -> 1st)
function count_str(s)
	local i = tonumber(s)
	if (i % 10) == 1 and i ~= 11 then
		return tostring(i) .. "st"
	elseif (i % 10) == 2 and i ~= 12 then
		return tostring(i) .. "nd"
	elseif (i % 10) == 3 and i ~= 13 then
		return tostring(i) .. "rd"
	end
	return tostring(i) .. "th"
end

-- Pretty printing of an age
function print_age(url, t)
	local now = os.time()
	local span = now - t
	local age = ""
	local name = pepper.utils.basename(url)

	-- Easy case: Birthday in the past
	if span < 0 then
		print(name .. " is not born yet!")
		return
	end

	-- Easy case: Happy birthday
	local today = string.format("%s %s", os.date("%B", now), count_str(os.date("%d", now)))
	local birthday = string.format("%s %s", os.date("%B", t), count_str(os.date("%d", t)))
	if today == birthday then
		local d = os.date("%Y", now) - os.date("%Y", t)
		if d == 1 then
			print(name .. " is " .. d .. " year old")
		elseif d ~= 0 then
			print(name .. " is " .. d .. " years old")
		end
		print("Happy birthday, " .. name .. "!")
		return
	end

	-- Age information
	if span < 60 then
		if span == 1 then age = "one second old" else age = tostring(span) .. " seconds old" end
	elseif span < 120 then age = "one minute old"
	elseif span < 45*60 then age = tonumber(os.date("%M", span)) .. " minutes old"
	elseif span < 24*60*60 then age = tonumber(os.date("%H", span)) .. " hours old"
	elseif span < 28*24*60*60 then age = tonumber(os.date("%d", span)) .. " days old"
	elseif span < 365*24*60*60 then age = tonumber(os.date("%m", span)) .. " months old"
	else
		local years = (tonumber(os.date("%Y", span)) - 1970)
		if years == 1 then age = "1 year old" else age = years .. " years old" end
	end

	-- Birthday information
	local birthday_date = os.date("*t", t)
	birthday_date.year = tonumber(os.date("%Y"))
	if os.time(birthday_date) - now < 0 then
		birthday_date.year = birthday_date.year + 1
	end
	local birthday_dist = pepper.datetime.humanrange(os.time(birthday_date) - now)

	-- Print result
	print(name .. " is " .. age)

	if today ~= birthday then
		local suffix = "'s"
		if name:sub(-1) == "s" then
			suffix = "'"
		end
		print(string.format("%s%s birthday is in %s (%s)", name, suffix, birthday_dist, birthday))
	elseif span > 60*60*24 then
	end
end

-- Main report function
function run(self)
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())

	if repo:type() == "mercurial" then
		print_age(repo:url(), repo:revision("0"):date())
	else
		local iterator = repo:iterator(branch, {prefetch=false})
		for revision in iterator:revisions() do
			print_age(repo:url(), revision:date())
			break
		end
	end
end
