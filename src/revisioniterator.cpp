/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
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

#include "revisioniterator.h"


// Constructor
RevisionIterator::RevisionIterator(Backend *backend, const std::string &branch, int64_t start, int64_t end)
	: m_backend(backend), m_total(0), m_consumed(0), m_atEnd(false)
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

	m_backend->prefetch(ids);
}
