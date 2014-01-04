/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: report.cpp
 * Report script context
 */


#include "main.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <stack>

#include "backend.h"
#include "diffstat.h"
#include "logger.h"
#include "luahelpers.h"
#include "luamodules.h"
#include "options.h"
#include "repository.h"
#include "revision.h"
#include "revisioniterator.h"
#include "strlib.h"
#include "tag.h"
#ifdef USE_GNUPLOT
 #include "plot.h"
#endif

#include "syslib/fs.h"

#include "report.h"

#define PEPPER_MAX_STACK_SIZE 64


/*
 * Stack helper functions
 */

namespace {

// Report entry function names
const char *funcs[] = {"run", "main", NULL};

// Checks if the given report is "executable", i.e. contains a main() function
bool isExecutable(lua_State *L, std::string *entryPoint = NULL)
{
	int i = 0;
	while (funcs[i] != NULL && !LuaHelpers::hasFunction(L, funcs[i])) {
		++i;
	}
	if (funcs[i] != NULL && entryPoint != NULL) {
		*entryPoint = funcs[i];
	}
	return funcs[i] != NULL;
}

// Prints a backtrace if Lua is panicking
int atpanic(lua_State *L)
{
	std::cerr << "Lua PANIC: " << LuaHelpers::tops(L) << std::endl;
#ifdef DEBUG
	std::cerr << PepperException::stackTrace();
#endif
	return 0;
}

// Returns all paths that may contains reports
std::vector<std::string> reportDirs()
{
	std::vector<std::string> dirs;

	// Read the PEPPER_REPORTS environment variable
	char *env = getenv("PEPPER_REPORTS");
	if (env != NULL) {
#ifdef POS_WIN
		std::vector<std::string> parts = str::split(env, ";");
#else
		std::vector<std::string> parts = str::split(env, ":");
#endif
		for (size_t i = 0; i < parts.size(); i++) {
			dirs.push_back(parts[i]);
		}
	}

#ifdef DATADIR
	dirs.push_back(DATADIR);
#endif
	return dirs;
}

// Returns the full path to the given script
std::string findScript(const std::string &script)
{
	if (sys::fs::fileExists(script)) {
		return script;
	}
	if (sys::fs::fileExists(script + ".lua")) {
		return script + ".lua";
	}

	std::vector<std::string> dirs = reportDirs();
	for (size_t i = 0; i < dirs.size(); i++) {
		std::string path = dirs[i] + "/" + script;
		if (sys::fs::fileExists(path)) {
			return path;
		}
		path += ".lua";
		if (sys::fs::fileExists(path)) {
			return path;
		}
	}
	return script;
}

// Sets up the lua context
lua_State *setupLua()
{
	// Setup lua context
	lua_State *L = lua_open();
	luaL_openlibs(L);

	lua_atpanic(L, atpanic);

	// Register extra modules functions
	LuaModules::registerModules(L);

	// Register binding classes
	Lunar<Report>::Register(L, "pepper");
	Lunar<Repository>::Register(L, "pepper");
	Lunar<Revision>::Register(L, "pepper");
	Lunar<RevisionIterator>::Register(L, "pepper");
	Lunar<Diffstat>::Register(L, "pepper");
	Lunar<Tag>::Register(L, "pepper");
#ifdef USE_GNUPLOT
	Lunar<Plot>::Register(L, "pepper");
#endif

	// Setup package path to include built-in modules
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "path");
	std::string path = LuaHelpers::pops(L);
	std::vector<std::string> dirs = reportDirs();
	for (size_t i = 0; i < dirs.size(); i++) {
		path += ";";
		path += dirs[i] + "/?.lua";
	}
	LuaHelpers::push(L, path);
	lua_setfield(L, -2, "path");
	lua_pop(L, 1);
	PTRACE << "Lua package path has been set to " << path << endl;

	// Setup (deprecated) meta table
	luaL_newmetatable(L, "meta");
	lua_setglobal(L, "meta");

	return L;
}

// Opens a lua script and returns its entry point
std::string loadReport(lua_State *L, const std::string &path)
{
	// Check script syntax by loading the file
	if (luaL_loadfile(L, path.c_str()) != 0) {
		throw PEX(lua_tostring(L, -1));
	}

	// Execute the main chunk
	if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0) {
		throw PEX(lua_tostring(L, -1));
	}

	std::string main;
	if (!isExecutable(L, &main)) {
		throw PEX("Not executable (no run function)");
	}
	return main;
}

// Wrapper for print(), mostly from VIM - Vi IMproved by Bram Moolenaar
int printWrapper(lua_State *L)
{
	std::string str;
	int n = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (int i = 1; i <= n; i++) {
		lua_pushvalue(L, -1); /* tostring */
		lua_pushvalue(L, i); /* arg */
		lua_call(L, 1, 1);
		size_t l;
		const char *s = lua_tolstring(L, -1, &l);
		if (s == NULL) {
			return luaL_error(L, "cannot convert to string");
		}

		if (i > 1) {
			str += '\t';
		}
		str += std::string(s, l);
		lua_pop(L, 1);
	}

	Report *c = Report::current();
	(c == NULL ? std::cout : c->out()) << str << std::endl;
	return 0;
}

} // anonymous namespace


/*
 * Report class implementation
 */

// Static report stack
std::stack<Report *> Report::s_stack;

// Constructor
Report::Report(const std::string &script, Backend *backend)
	: m_repo(NULL), m_script(script), m_out(&std::cout),  m_metaDataRead(false)
{
	if (backend) {
		m_repo = new Repository(backend);
		m_options = backend->options().reportOptions();
	}
}

// Constructor
Report::Report(const std::string &script, const std::map<std::string, std::string> &options, Backend *backend)
	: m_repo(NULL), m_script(script), m_options(options), m_out(&std::cout), m_metaDataRead(false)
{
	if (backend) {
		m_repo = new Repository(backend);
	}	
}

// Destructor
Report::~Report()
{
	delete m_repo;
}


// Runs the report, printing error messages to the given stream
int Report::run(std::ostream &out, std::ostream &err)
{
	if (s_stack.size() == PEPPER_MAX_STACK_SIZE) {
		err << "Error running report: maximum stack size (";
		err << PEPPER_MAX_STACK_SIZE << ") exceeded" << std::endl;
		return EXIT_FAILURE;
	}

	// Push this context on top of the stack
	std::string path = findScript(m_script);
	PDEBUG << "Report path resolved: " << m_script << " -> " << path << endl;
	PTRACE << "Pushing report context for " << path << endl;
	s_stack.push(this);
	PDEBUG << "Stack size is " << s_stack.size() << endl;

	std::ostream *prevout = m_out;
	m_out = &out;

	// Ensure the backend is ready
	m_repo->backend()->open();

	lua_State *L = setupLua();

	// Wrap print() function to use custom output stream
	lua_pushcfunction(L, printWrapper);
	lua_setglobal(L, "print");

	// Run the script
	std::string main;
	int ret = EXIT_SUCCESS;
	try {
		main = loadReport(L, path);
	} catch (const std::exception &ex) {
		err << "Error opening report: " << ex.what() << std::endl;
		goto error;
	}

	// Call the report function, with the current report context as an argument
	lua_getglobal(L, main.c_str());
	LuaHelpers::push(L, this);
	if (lua_pcall(L, 1, 1, 0) != 0) {
		err << "Error running report: " << lua_tostring(L, -1) << std::endl;
		goto error;
	}
	goto cleanup;

error:
	ret = EXIT_FAILURE;
cleanup:
	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);

	// Inform backend that the report is done
	m_repo->backend()->close();

	PTRACE << "Popping report context for " << path <<  endl;
	s_stack.pop();
	m_out = prevout;
	return ret;
}

// Pretty-prints report options
void Report::printHelp()
{
	if (!m_metaDataRead) {
		readMetaData();
	}

	std::cout << "Options for report '" << m_metaData.name << "':" << std::endl;

	for (size_t i = 0; i < m_metaData.options.size(); i++) {
		Options::print(m_metaData.options[i].synopsis, m_metaData.options[i].description);
	}
}

// Returns the report's meta data
Report::MetaData Report::metaData()
{
	if (!m_metaDataRead) {
		readMetaData();
	}

	return m_metaData;
}

// Checks if the report is valid (e.g., executable)
bool Report::valid()
{
	std::string path = findScript(m_script);
	lua_State *L = setupLua();

	bool valid = true;
	try {
		loadReport(L, path);
	} catch (const std::exception &ex) {
		PDEBUG << "Invalid report: " << ex.what() << endl;
		valid = false;
	}

	// Clean up
	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);
	return valid;
}

// Returns the report output stream
std::ostream &Report::out() const
{
	return *m_out;
}

// Returns whether the standard output is redirected
bool Report::outputRedirected() const
{
	return (m_out != &std::cout);
}

// Lists all report scripts and their descriptions
std::vector<std::pair<std::string, std::string> > Report::listReports()
{
	std::vector<std::pair<std::string, std::string> > reports;
	std::vector<std::string> dirs = reportDirs();
	for (size_t j = 0; j < dirs.size(); j++) {
		std::string builtin = dirs[j];
		if (!sys::fs::dirExists(builtin)) {
			continue;
		}

		std::vector<std::string> contents = sys::fs::ls(builtin);
		std::sort(contents.begin(), contents.end());
		for (size_t i = 0; i < contents.size(); i++) {
			if (contents[i].empty() || contents[i][contents[i].length()-1] == '~' || !sys::fs::fileExists(builtin + "/" + contents[i])) {
				continue;
			}

			std::string path = builtin + "/" + contents[i];
			Report report(path);
			if (report.valid()) {
				reports.push_back(std::pair<std::string, std::string>(path, report.metaData().description));
			}
		}
	}

	return reports;
}

// Prints a listing of all report scripts to stdout
void Report::printReportListing(std::ostream &out)
{
	std::vector<std::pair<std::string, std::string> > reports = listReports();
	std::string dirname;
	for (size_t i = 0; i < reports.size(); i++) {
		std::string nextdir = sys::fs::dirname(reports[i].first);
		if (dirname != nextdir) {
			dirname = nextdir;
			if (i < reports.size()-1) {
				out << std::endl;
			}
			out << "Available reports in " << dirname << ":" << std::endl;
		}

		std::string name = sys::fs::basename(reports[i].first);
		if (name.length() > 4 && name.compare(name.length()-4, 4, ".lua") == 0) {
			name = name.substr(0, name.length()-4);
		}

		Options::print(name, reports[i].second, out);
	}

#ifndef USE_GNUPLOT
	out << std::endl << "NOTE: Built without Gnuplot support. ";
	out << "Graphical reports are not shown." << std::endl;
#endif
}

// Returns the backend wrapper
Repository *Report::repository() const
{
	return m_repo;
}

// Returns the report options
std::map<std::string, std::string> Report::options() const
{
	return m_options;
}

// Returns the current context
Report *Report::current()
{
	return (s_stack.empty() ? NULL : s_stack.top());
}

// Reads the report script's meta data
void Report::readMetaData()
{
	lua_State *L = setupLua();

	// Open the script
	std::string path = findScript(m_script);
	try {
		loadReport(L, path);
	} catch (const std::exception &ex) {
		throw PEX(str::printf("Error opening report: %s", ex.what()));
	}

	// Try to run describe()
	lua_getglobal(L, "describe");
	if (lua_type(L, -1) == LUA_TFUNCTION) {
		LuaHelpers::push(L, this);
		if (lua_pcall(L, 1, 1, 0) != 0) {
			throw PEX(str::printf("Error opening report: %s", lua_tostring(L, -1)));
		}
		if (lua_type(L, -1) != LUA_TTABLE) {
			throw PEX("Error opening report: Expected table from describe()");
		}
	} else {
		lua_pop(L, 1);
		// Else, use the meta table
		lua_getglobal(L, "meta");
		if (lua_type(L, -1) != LUA_TTABLE) {
			throw PEX("Error opening report: Neither describe() nor meta-table found");
		}
	}

	// Read the report name
	m_metaData.name = sys::fs::basename(m_script);
	lua_getfield(L, -1, "title");
	if (lua_type(L, -1) == LUA_TSTRING) {
		m_metaData.name = LuaHelpers::tops(L);
	} else {
		lua_pop(L, 1);
		// "meta.name" has been used in the first version of the scripting tutorial
		lua_getfield(L, -1, "name");
		if (lua_type(L, -1) == LUA_TSTRING) {
			m_metaData.name = LuaHelpers::tops(L);
		}
	}
	lua_pop(L, 1);

	// Read the report description
	lua_getfield(L, -1, "description");
	if (lua_type(L, -1) == LUA_TSTRING) {
		m_metaData.description = LuaHelpers::tops(L);
	}
	lua_pop(L, 1);

	// Check for possible options
	lua_getfield(L, -1, "options");
	if (lua_type(L, -1) == LUA_TTABLE) {
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			if (lua_type(L, -1) != LUA_TTABLE) {
				lua_pop(L, 1);
				continue;
			}

			int i = 0;
			std::string arg, text;

			lua_pushnil(L);
			while (lua_next(L, -2) != 0) {
				if (lua_type(L, -1) == LUA_TSTRING) {
					if (i == 0) {
						arg = luaL_checkstring(L, -1);
					} else if (i == 1) {
						text = luaL_checkstring(L, -1);
					}
				}
				++i;
				lua_pop(L, 1);
			}
			lua_pop(L, 1);

			if (i >= 2) {
				m_metaData.options.push_back(MetaData::Option(arg, text));
			}
		}
	}
	lua_pop(L, 1);

	// Clean up
	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);
	m_metaDataRead = true;
}

/*
 * Lua binding
 */

const char Report::className[] = "report";
Lunar<Report>::RegType Report::methods[] = {
	LUNAR_DECLARE_METHOD(Report, getopt),
	LUNAR_DECLARE_METHOD(Report, repository),
	LUNAR_DECLARE_METHOD(Report, run),
	LUNAR_DECLARE_METHOD(Report, name),
	LUNAR_DECLARE_METHOD(Report, description),
	LUNAR_DECLARE_METHOD(Report, options),
	{0,0}
};

Report::Report(lua_State *L)
	: m_repo(NULL), m_out(&std::cout), m_metaDataRead(false)
{
	if (lua_gettop(L) != 1 && lua_gettop(L) != 2) {
		LuaHelpers::pushError(L, "Invalid number of arguments (1 or 2 expected)");
		return;
	}
	if (!Report::current()) {
		LuaHelpers::pushError(L, "No current report");
		return;
	}

	m_options = Report::current()->options();
	if (lua_gettop(L) == 2) {
		m_options = LuaHelpers::popms(L);
	}
	m_script = LuaHelpers::pops(L);

	m_repo = new Repository(Report::current()->repository()->backend());
}

int Report::repository(lua_State *L)
{
	return LuaHelpers::push(L, m_repo);
}

int Report::getopt(lua_State *L)
{
	if (lua_gettop(L) != 1 && lua_gettop(L) != 2) {
		return luaL_error(L, "Invalid number of arguments (1 or 2 expected)");
	}

	bool defaultProvided = (lua_gettop(L) == 2);
	std::string defaultValue = (lua_gettop(L) == 2 ? LuaHelpers::pops(L) : std::string());
	std::vector<std::string> keys = str::split(LuaHelpers::pops(L), ",", true);

	std::string value;
	bool valueFound = false;
	for (unsigned int i = 0; i < keys.size(); i++) {
		if (m_options.find(keys[i]) != m_options.end()) {
			value = m_options.find(keys[i])->second;
			valueFound = true;
			break;
		}
	}

	if (valueFound) {
		return LuaHelpers::push(L, value);
	} else if (defaultProvided) {
		return LuaHelpers::push(L, defaultValue);
	}
	return LuaHelpers::pushNil(L);
}

int Report::run(lua_State *L)
{
	try {
		std::stringstream out, err;
		if (run(out, err) != 0) {
			return LuaHelpers::pushError(L, str::trim(err.str()));
		}
		return LuaHelpers::push(L, out.str());
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	} catch (const std::exception &ex) {
		return LuaHelpers::pushError(L, ex.what());
	}
	return LuaHelpers::pushNil(L);
}

int Report::name(lua_State *L)
{
	try {
		if (!m_metaDataRead) {
			readMetaData();
		}
		return LuaHelpers::push(L, m_metaData.name);
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	} catch (const std::exception &ex) {
		return LuaHelpers::pushError(L, ex.what());
	}
	return LuaHelpers::pushNil(L);
}

int Report::description(lua_State *L)
{
	try {
		if (!m_metaDataRead) {
			readMetaData();
		}
		return LuaHelpers::push(L, m_metaData.description);
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	} catch (const std::exception &ex) {
		return LuaHelpers::pushError(L, ex.what());
	}
	return LuaHelpers::pushNil(L);
}

int Report::options(lua_State *L)
{
	try {
		if (!m_metaDataRead) {
			readMetaData();
		}

		// Convert each option into a 2-dimensional vector
		std::vector<std::vector<std::string> > opvec;
		for (size_t i = 0; i < m_metaData.options.size(); i++) {
			std::vector<std::string> v;
			v.push_back(m_metaData.options[i].synopsis);
			v.push_back(m_metaData.options[i].description);
			opvec.push_back(v);
		}
		return LuaHelpers::push(L, opvec);
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	} catch (const std::exception &ex) {
		return LuaHelpers::pushError(L, ex.what());
	}
	return LuaHelpers::pushNil(L);
}
