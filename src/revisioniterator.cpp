/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: revisioniterator.cpp
 * Base class for revision iterators
 *
 * The RevisionIterator is being used for walking through version history.
 * The main design idea is to make iteration as fast as possible, resulting
 * in two properties:
 *
 *    * The revsions that are included in the iteration are determined in a
 *      seperate thread, concurrent to the backend data fetching operations.
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
RevisionIterator::RevisionIterator(const std::string &branch, Backend *backend)
	: m_backend(backend), m_index(0), m_atEnd(false)
{
	m_logIterator = backend->iterator(branch);
}

// Destructor
RevisionIterator::~RevisionIterator()
{
	delete m_logIterator;
}

// Returns whether the iterator is at its end
bool RevisionIterator::atEnd()
{
	if (m_buffer.empty()) {
		// Iteration has just started, and we don't know whether there are
		// any revisions at all
		fetchLogs();
	}

	return m_atEnd;
}

// Returns the next revision
std::string RevisionIterator::next()
{
	// Call the function in order to retrieve the first revisions if necessary
	if (atEnd()) {
		return std::string();
	}
	if (m_index >= m_buffer.size()-1) {
		fetchLogs();
	}
	return (m_index < m_buffer.size() ? m_buffer[m_index++] : std::string());
}

// Fetches new logs
void RevisionIterator::fetchLogs()
{
	std::vector<std::string> next = m_logIterator->nextIds();
	if (next.empty()) {
		m_atEnd = true;
	} else {
		m_backend->prefetch(next);
		for (unsigned int i = 0; i < next.size(); i++) {
			m_buffer.push_back(next[i]);
		}
	}
}
