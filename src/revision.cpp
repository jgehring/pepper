/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: revision.cpp
 * Revision data class
 */


#include <iostream>

#include "bstream.h"

#include "revision.h"


// Constructor
Revision::Revision(const std::string &id)
	: m_id(id), m_date(0)
{

}

// Constructor
Revision::Revision(const std::string &id, int64_t date, const std::string &author, const std::string &message, const Diffstat &diffstat)
	: m_id(id), m_date(date), m_author(author), m_message(message), m_diffstat(diffstat)
{

}

// Destructor
Revision::~Revision()
{

}

// Returns the revision ID (e.g., the revision number)
std::string Revision::id() const
{
	return m_id;
}

// Returns the diffstat object
Diffstat Revision::diffstat() const
{
	return m_diffstat;
}

// Writes the revision to a binary stream (not writing the ID)
void Revision::write(BOStream &out) const
{
	out << m_date << m_author << m_message;
	m_diffstat.write(out);
}

// Loads the revision from a binary stream (not changing the ID)
bool Revision::load(BIStream &in)
{
	in >> m_date >> m_author >> m_message;
	return m_diffstat.load(in);
}
