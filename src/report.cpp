/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: report.cpp
 * Report script access and invocation, as well as a Lua interface proiding
 * main report actions
 */


#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "backend.h"
#include "diffstat.h"
#include "logger.h"
#include "options.h"
#include "repository.h"
#include "revision.h"
#include "revisioniterator.h"
#include "tag.h"
#include "utils.h"

#include "luahelpers.h"
#ifdef USE_GNUPLOT
 #include "plot.h"
#endif

#include "syslib/fs.h"

#include "report.h"


namespace report
{

// Global variables
static Repository *repo;
static std::map<std::string, std::string> options;


// Returns the current repository
static int repository(lua_State *L)
{
	return LuaHelpers::push(L, repo);
}

// Returns a script option (or the default value)
static int getopt(lua_State *L)
{
	if (lua_gettop(L) != 1 && lua_gettop(L) != 2) {
		return luaL_error(L, "Invalid number of arguments (1 or 2 expected)");
	}

	int narg = lua_gettop(L);

	std::vector<std::string> keys = utils::split(LuaHelpers::tops(L, (narg == 1 ? -1 : -2)), ",", true);
	std::string defaultValue = (narg == 2 ? LuaHelpers::tops(L, -1) : std::string());

	std::string value;
	bool valueFound = false;
	for (unsigned int i = 0; i < keys.size(); i++) {
		if (options.find(keys[i]) != options.end()) {
			value = options[keys[i]];
			valueFound = true;
			break;
		}
	}

	lua_pop(L, narg);
	if (valueFound) {
		return LuaHelpers::push(L, value);
	} else if (narg == 2) {
		return LuaHelpers::push(L, defaultValue);
	}
	return LuaHelpers::pushNil(L);
}

// Returns the revision specified by the given ID
static int revision(lua_State *L)
{
	std::string id = LuaHelpers::pops(L);
	Revision *revision = NULL;
	try {
		revision = repo->backend()->revision(id);
	} catch (const Pepper::Exception &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}
	return LuaHelpers::push(L, revision); // TODO: Memory leak!
}

// Maps a lua function on all revisions of a given branch
static int walk_branch(lua_State *L)
{
	if (lua_gettop(L) != 2) {
		return luaL_error(L, "Invalid number of arguments (2 expected)");
	}

	luaL_checktype(L, -2, LUA_TFUNCTION);
	std::string branch = LuaHelpers::pops(L);
	int callback = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pop(L, 1);

	Logger::status() << "Initializing iterator... " << flush;

	Backend *backend = repo->backend();
	RevisionIterator *it = NULL;
	try {
		it = new RevisionIterator(branch, backend);
	} catch (const Pepper::Exception &ex) {
		Logger::status() << "failed" << endl;
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}
	Logger::status() << "done" << endl;

	if (Logger::level() < Logger::Info) {
		Logger::status() << "Fetching revisions... " << flush;
	}
	while (!it->atEnd()) {
		Revision *revision = NULL;
		try {
			revision = backend->revision(it->next());
		} catch (const Pepper::Exception &ex) {
			delete it;
			return LuaHelpers::pushError(L, ex.what(), ex.where());
		}

		PTRACE << "Fetched revision " << revision->id() << endl;

		lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
		LuaHelpers::push(L, revision);
		lua_call(L, 1, 1);
		lua_pop(L, 1);

		Logger::info() << "\r\033[0K";
		Logger::info() << "Fetching revisions... " << revision->id() << flush;
		delete revision;
	}

	Logger::info() << "\r\033[0K";
	Logger::info() << "Fetching revisions... done" << endl;
	if (Logger::level() < Logger::Info) {
		Logger::status() << "done" << endl;
	}

	delete it;
	try {
		backend->finalize();
	} catch (const Pepper::Exception &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}
	return 0;
}

// Function table of the report library
static const struct luaL_reg report[] = {
	{"getopt", getopt},
	{"repository", repository},
	{"revision", revision},
	{"walk_branch", walk_branch},
	{NULL, NULL}
};


// Custom fclose() handler for lua file handles
static int utils_fclose(lua_State *L)
{
	FILE **p = (FILE **)lua_touserdata(L, 1);
	int rc = fclose(*p);
	if (rc == 0) *p = NULL;
	return 1;
}

// Generates a temporary file, and returns a file handle as well as the file name
static int utils_mkstemp(lua_State *L)
{
	std::string templ;
	if (lua_gettop(L) > 0) {
		templ = LuaHelpers::pops(L);
	} else {
		templ = "/tmp/pepperXXXXXX";
	}

	char *buf = strdup(templ.c_str());
	int fd = mkstemp(buf);

	FILE **pf = (FILE **)lua_newuserdata(L, sizeof *pf);
	*pf = 0;
	luaL_getmetatable(L, LUA_FILEHANDLE);
	lua_setmetatable(L, -2);

	// Register custom __close() function
	// (From lua posix module by Luiz Henrique de Figueiredo)
	lua_getfield(L, LUA_REGISTRYINDEX, "PEPPER_UTILS_FILE");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_pushcfunction(L, utils_fclose);
		lua_setfield(L, -2, "__close");
		lua_setfield(L, LUA_REGISTRYINDEX, "PEPPER_UTILS_FILE");
	}
	lua_setfenv(L, -2);

	*pf = fdopen(fd, "r+w");
	LuaHelpers::push(L, (const char *)buf);
	free(buf);
	return 2;
}

// Removes a file
static int utils_unlink(lua_State *L)
{
	if (lua_gettop(L) != 1) {
		return luaL_error(L, "Invalid number of arguments (1 expected)");
	}
	unlink(LuaHelpers::tops(L).c_str());
	return 0;
}

// Splits a string
static int utils_split(lua_State *L)
{
	if (lua_gettop(L) != 2) {
		return luaL_error(L, "Invalid number of arguments (2 expected)");
	}

	std::string pattern = LuaHelpers::pops(L);
	std::string string = LuaHelpers::pops(L);
	return LuaHelpers::push(L, utils::split(string, pattern));
}

// Function table of the utils library
static const struct luaL_reg utils[] = {
	// TODO: Error handling for all functions
	{"mkstemp", utils_mkstemp},
	{"unlink", utils_unlink},
	{"split", utils_split},
	{NULL, NULL}
};


// Sets up the lua context
lua_State *setupLua()
{
	// Setup lua context
	lua_State *L = lua_open();
	luaL_openlibs(L);

	// Register report functions
	luaL_register(L, "pepper.report", report);
	luaL_register(L, "pepper.utils", utils);

	// Register binding classes
	Lunar<Repository>::Register(L, "pepper");
	Lunar<Revision>::Register(L, "pepper");
	Lunar<Diffstat>::Register(L, "pepper");
	Lunar<Tag>::Register(L, "pepper");
#ifdef USE_GNUPLOT
	Lunar<Plot>::Register(L, "pepper");
#endif

	// Setup meta table
	luaL_newmetatable(L, "meta");
	lua_setglobal(L, "meta");

	return L;
}

// Returns all paths that may contains reports
static std::vector<std::string> reportDirs()
{
	std::vector<std::string> dirs;
#ifdef DATADIR
	dirs.push_back(DATADIR);
#endif

	// Read the PEPPER_REPORTS environment variable
	char *env = getenv("PEPPER_REPORTS");
	if (env != NULL) {
#ifdef POS_WIN
		std::vector<std::string> parts = utils::split(env, ";");
#else
		std::vector<std::string> parts = utils::split(env, ":");
#endif
		for (size_t i = 0; i < parts.size(); i++) {
			dirs.push_back(parts[i]);
		}
	}

	return dirs;
}

// Returns the full path to the given script
std::string findScript(const std::string &script)
{
	if (sys::fs::exists(script)) {
		return script;
	}
	if (sys::fs::exists(script + ".lua")) {
		return script + ".lua";
	}

	std::vector<std::string> dirs = reportDirs();
	for (size_t i = 0; i < dirs.size(); i++) {
		std::string path = dirs[i] + "/" + script;
		if (sys::fs::exists(path)) {
			return path;
		}
		path += ".lua";
		if (sys::fs::exists(path)) {
			return path;
		}
	}
	return script;
}

// Runs a scripted report using the given backend
int run(const std::string &script, Backend *backend)
{
	lua_State *L = setupLua();

	// Setup global variables
	report::repo = new Repository(backend);
	report::options = backend->options().scriptOptions();

	// Run the script
	int ret = EXIT_SUCCESS;
	std::string path = findScript(script);
	if (luaL_dofile(L, path.c_str()) != 0) {
		std::cerr << "Error opening report: " << lua_tostring(L, -1) << std::endl;
		ret = EXIT_FAILURE;
	} else {
		// Call the report function
		lua_getglobal(L, "main");
		if (lua_pcall(L, 0, 1, 0) != 0) {
			std::cerr << "Error running report: " << lua_tostring(L, -1) << std::endl;
			ret = EXIT_FAILURE;
		}
	}

	// Clean up
	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);
	delete report::repo;
	return ret;
}

// Opens a script, calls the "options()" function and pretty-prints some help
void printHelp(const std::string &script)
{
	lua_State *L = setupLua();

	// Open the script
	std::string path = findScript(script);
	if (luaL_dofile(L, path.c_str()) != 0) {
		throw PEX(utils::strprintf("Error opening report: %s", lua_tostring(L, -1)));
	}

	lua_getglobal(L, "meta");

	// Retrieve the report name
	std::string name = script;
	lua_getfield(L, -1, "title");
	if (lua_type(L, -1) == LUA_TSTRING) {
		name = LuaHelpers::pops(L);
	}

	// Check for possible options
	lua_getfield(L, -1, "options");
	if (lua_type(L, -1) == LUA_TTABLE) {
		// Pop the arguments from the stack
		std::vector<std::string> switches, text;
		luaL_checktype(L, -1, LUA_TTABLE);
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			luaL_checktype(L, -1, LUA_TTABLE);
			lua_pushnil(L);
			int i = 0;
			while (lua_next(L, -2) != 0) {
				if (i == 0) {
					switches.push_back(luaL_checkstring(L, -1));
				} else if (i == 1) {
					text.push_back(luaL_checkstring(L, -1));
				}
				++i;
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
		}
		lua_pop(L, 1);

		std::cout << "Options for report '" << name << "':" << std::endl;
		for (unsigned int i = 0; i < std::min(switches.size(), text.size()); i++) {
			Options::print(switches[i], text[i]);
		}
	} else {
		std::cout << "No options for report '" << name << "':" << std::endl;
	}
	lua_pop(L, 1);

	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);
}

// Prints a listing of all report scripts to stdout
void listReports(std::ostream &out)
{
	std::vector<std::string> dirs = reportDirs();
	for (size_t j = 0; j < dirs.size(); j++) {
		std::string builtin = dirs[j];
		out << "Available reports in " << builtin << ":" << std::endl;

		std::vector<std::string> reports = sys::fs::ls(builtin);
		std::sort(reports.begin(), reports.end());

		for (size_t i = 0; i < reports.size(); i++) {
			if (reports[i].empty() || reports[i][reports[i].length()-1] == '~' || !sys::fs::fileExists(builtin + "/" + reports[i])) {
				continue;
			}

			lua_State *L = setupLua();
			std::string path = builtin + "/" + reports[i];
			if (luaL_dofile(L, path.c_str()) != 0) {
				PDEBUG << "Error opening report at " << path << ": " << lua_tostring(L, -1) << endl;
				lua_gc(L, LUA_GCCOLLECT, 0);
				lua_close(L);
				continue;
			}

			lua_getglobal(L, "meta");

			// Retrieve the report description
			std::string description;
			lua_getfield(L, -1, "description");
			if (lua_type(L, -1) == LUA_TSTRING) {
				description = LuaHelpers::pops(L);
			} else {
				lua_pop(L, 1);
			}

			// Strip possible Lua suffix when displaying
			std::string name = reports[i];
			if (name.length() > 4 && name.compare(name.length()-4, 4, ".lua") == 0) {
				name = name.substr(0, name.length()-4);
			}

			Options::print(reports[i], description, out);
			lua_gc(L, LUA_GCCOLLECT, 0);
			lua_close(L);
		}

		if (j < dirs.size()-1) {
			out << std::endl;
		}
	}
}

} // namespace report
