/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: revision.cpp
 * Revision data class
 */


#include "revision.h"


// Utiliy function for getting values out of a map without inserting new ones
static inline std::string mapget(const std::map<std::string, std::string> &map, const std::string &key)
{
	std::map<std::string, std::string>::const_iterator it = map.find(key);
	if (it != map.end()) {
		return (*it).second;
	}
	return std::string();
}


// Constructor
Revision::Revision(const std::string &id)
	: m_id(id)
{

}

// Destructor
Revision::~Revision()
{

}

// Queries
std::string Revision::id() const
{
	return m_id;
}

std::string Revision::date() const
{
	return mapget(m_data, "date");
}

std::string Revision::author() const
{
	return mapget(m_data, "author");
}
