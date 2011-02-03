/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
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
#include <vector>

#include "backend.h"


class RevisionIterator
{
	public:
		RevisionIterator(const std::string &branch, Backend *backend);
		~RevisionIterator();

		bool atEnd();
		std::string next();

		int progress() const;

	private:
		void fetchLogs();

	protected:
		Backend *m_backend;
		Backend::LogIterator *m_logIterator;
		std::vector<std::string> m_buffer;
		std::vector<std::string>::size_type m_index;
		bool m_atEnd;
};


#endif // REVISIONITERATOR_H_
