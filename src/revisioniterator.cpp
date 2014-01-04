/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: revisioniterator.cpp
 * Base class for revision iterators
 *
 * The RevisionIterator is being used for walking through version history.
 * The main design idea is to make iteration as fast as possible, resulting
 * in two properties:
 *
 *    * The revsions that are included in the iteration are determined in a
 *      separate thread, concurrent to the backend data fetching operations.
 *    * Backends can pre-fetch revision data, probably using multiple threads
 *
 * The backend provides a LogIterator thread which offers methods for retrieving 
 * a bunch of revisions from the repository log at once. Once these revisions
 * are known, the backend can pre-fetch them if it needs to. Note that the
 * RevisionIterator calls the corresponding backend methods for pre-fetching.
 * This design isn't exceptionally beautiful but functional. 
 */


#include "main.h"

#include "logger.h"
#include "luahelpers.h"
#include "revision.h"

#include "revisioniterator.h"


// Constructor
RevisionIterator::RevisionIterator(Backend *backend, const std::string &branch, int64_t start, int64_t end, Flags flags)
	: m_backend(backend), m_total(0), m_consumed(0), m_atEnd(false), m_flags(flags)
{
	m_logIterator = backend->iterator(branch, start, end);
	m_logIterator->start();
}

// Destructor
RevisionIterator::~RevisionIterator()
{
	m_logIterator->wait();
	delete m_logIterator;
}

// Returns whether the iterator is at its end
bool RevisionIterator::atEnd()
{
	if (m_total == 0) {
		// Iteration has just started, and we don't know whether there are
		// any revisions at all
		fetchLogs();
	}

	return m_queue.empty();
}

// Returns the next revision
std::string RevisionIterator::next()
{
	// Call the function in order to retrieve the first revisions if necessary
	if (atEnd()) {
		return std::string();
	}
	if (m_queue.size() <= 1) {
		fetchLogs();
		if (m_queue.empty()) {
			return std::string();
		}
	}

	++m_consumed;
	std::string id = m_queue.front();
	m_queue.pop();
	return id;
}

// Returns a progress estimate (percejjjge)
int RevisionIterator::progress() const
{
	if (m_logIterator->running()) {
		return 0;
	}
	return int((100.0f * m_consumed) / m_total);
}

// Fetches new logs
void RevisionIterator::fetchLogs()
{
	// We need to ask the backend to prefetch the next revision, so
	// a temporary queue is used for fetching the next IDs
	std::queue<std::string> tq;
	m_logIterator->nextIds(&tq);

	m_total += tq.size();
	std::vector<std::string> ids;
	while (!tq.empty()) {
		ids.push_back(tq.front());
		m_queue.push(tq.front());
		tq.pop();
	}

	if (m_flags & PrefetchRevisions) {
		m_backend->prefetch(ids);
	}
}

/*
 * Lua binding
 */

const char RevisionIterator::className[] = "iterator";
Lunar<RevisionIterator>::RegType RevisionIterator::methods[] = {
	LUNAR_DECLARE_METHOD(RevisionIterator, next),
	LUNAR_DECLARE_METHOD(RevisionIterator, revisions),
	LUNAR_DECLARE_METHOD(RevisionIterator, map),
	{0,0}
};

RevisionIterator::RevisionIterator(lua_State *)
{
	m_backend = NULL;
	m_logIterator = NULL;
}

int RevisionIterator::next(lua_State *L)
{
	if (atEnd()) {
		return LuaHelpers::pushNil(L);
	}	

	Revision *revision = NULL;
	try {
		revision = m_backend->revision(next());
		m_backend->filterDiffstat(&(revision->m_diffstat));
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}

	PTRACE << "Fetched revision " << revision->id() << endl;
	return LuaHelpers::push(L, revision);
}

// Static function that will call the next() function from above. It is being
// used as an iterator in RevisionIterator::revisions()
static int callnext(lua_State *L)
{
	RevisionIterator *it = LuaHelpers::topl<RevisionIterator>(L, -2);
	return it->next(L);
}

int RevisionIterator::revisions(lua_State *L)
{
	LuaHelpers::push(L, callnext);
	LuaHelpers::push(L, this);
	LuaHelpers::pushNil(L);
	return 3;
}

int RevisionIterator::map(lua_State *L)
{
	if (m_backend == NULL) return LuaHelpers::pushNil(L);

	if (lua_gettop(L) != 1) {
		return luaL_error(L, "Invalid number of arguments (1 expected)");
	}

	luaL_checktype(L, -1, LUA_TFUNCTION);
	int callback = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pop(L, 1);

	int progress = 0;
	if (Logger::level() < Logger::Info) {
		Logger::status() << "Fetching revisions... " << flush;
	}
	while (!atEnd()) {
		Revision *revision = NULL;
		try {
			revision = m_backend->revision(next());
			m_backend->filterDiffstat(&(revision->m_diffstat));
		} catch (const PepperException &ex) {
			return LuaHelpers::pushError(L, ex.what(), ex.where());
		}

		PTRACE << "Fetched revision " << revision->id() << endl;

		lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
		LuaHelpers::push(L, revision);
		lua_call(L, 1, 1);
		lua_pop(L, 1);

		if (Logger::level() > Logger::Info) {
			Logger::info() << "\r\033[0K";
			Logger::info() << "Fetching revisions... " << revision->id() << flush;
		} else {
			if (progress != this->progress()) {
				progress = this->progress();
				Logger::status() << "\r\033[0K";
				Logger::status() << "Fetching revisions... " << progress << "%" << flush;
			}
		}
		delete revision;
	}

	Logger::status() << "\r\033[0K";
	Logger::status() << "Fetching revisions... done" << endl;

	try {
		m_backend->finalize();
	} catch (const PepperException &ex) {
		return LuaHelpers::pushError(L, ex.what(), ex.where());
	}
	return 0;
}
