--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2012 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: check_cache.lua
	Runs a cache check
--]]


-- Describes the report
function describe(self)
	local r = {}
	r.title = "Cache check"
	r.description = "Checks and cleans up the revision cache"
	r.options = {{"-f,--force", "Force clearing of cache if necessary"}}
	return r
end

-- Main script function
function run(self)
	pepper.internal.check_cache(self:repository(), self:getopt("f,force"))
end
