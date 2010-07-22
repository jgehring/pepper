/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: revision.h
 * Revision data class (interface)
 */


#ifndef REVISION_H_
#define REVISION_H_


#include <map>
#include <string>


class Revision
{
	public:
		Revision(const std::string &id);
		~Revision();

		std::string id() const;
		std::string date() const;
		std::string author() const;

	private:
		std::string m_id;
		std::map<std::string, std::string> m_data;
};


#endif // REVISION_H_
