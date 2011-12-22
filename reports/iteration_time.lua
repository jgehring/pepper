require "pepper.datetime"

-- Describes the report
function describe(self)
	local r = {}
	r.name = "iteration-time"
	r.description = "Measures rveision iteration time"
	r.options = {
		{"-bARG, --branch=ARG", "Select branch"}
	}
	pepper.datetime.add_daterange_options(r)
	return r
end

-- Main report function
function run(self)
	local repo = self:repository()
	local branch = self:getopt("b,branch", repo:default_branch())
	local datemin, datemax = pepper.datetime.date_range(self)

	local w = pepper.watch()
	repo:iterator(branch, {start=datemin, stop=datemax}):map(function (r) print("* Fetched revision " .. r:id() .. ".") end)
	print(string.format("elapsed time: %.4fs", w:elapsed()))
end
