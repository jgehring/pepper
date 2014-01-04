/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: revisioniterator.h
 * Base class for revision iterators (interface)
 */


#ifndef REVISIONITERATOR_H_
#define REVISIONITERATOR_H_


#include <string>
#include <queue>

#include "backend.h"

#include "lunar/lunar.h"


class RevisionIterator
{
	public:
		enum Flags {
			PrefetchRevisions = 0x01
		};

	public:
		RevisionIterator(Backend *backend, const std::string &branch = std::string(), int64_t start = -1, int64_t end = -1, Flags flags = PrefetchRevisions);
		~RevisionIterator();

		bool atEnd();
		std::string next();

		int progress() const;

	private:
		void fetchLogs();

	protected:
		Backend *m_backend;
		Backend::LogIterator *m_logIterator;
		std::queue<std::string> m_queue;
		std::queue<std::string>::size_type m_total, m_consumed;
		bool m_atEnd;
		Flags m_flags;

	// Lua binding
	public:
		RevisionIterator(lua_State *L);

		int next(lua_State *L);
		int revisions(lua_State *L);
		int map(lua_State *L);

		static const char className[];
		static Lunar<RevisionIterator>::RegType methods[];
};


#endif // REVISIONITERATOR_H_
