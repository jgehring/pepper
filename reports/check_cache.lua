--[[
	pepper - SCM statistics report generator
	Copyright (C) 2010-2011 Jonas Gehring

	Released under the GNU General Public License, version 3.
	Please see the COPYING file in the source distribution for license
	terms and conditions, or see http://www.gnu.org/licenses/.

	file: check_cache.lua
	Runs a cache check
--]]

-- Script meta-data
meta.title = "Cache check"
meta.description = "Checks and cleans up the revision cache"


-- Main script function
function main()
	pepper.internal.check_cache(pepper.report.repository())
end
