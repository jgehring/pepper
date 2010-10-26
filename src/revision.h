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

#include "main.h"

#include "diffstat.h"

class BIStream;
class BOStream;


class Revision
{
	friend class LuaRevision;

	public:
		Revision(const std::string &id);
		Revision(const std::string &id, int64_t date, const std::string &author, const std::string &message, const Diffstat &diffstat);
		~Revision();

		std::string id() const;
		Diffstat diffstat() const;

		void write(BOStream &out) const;
		bool load(BIStream &in);

	private:
		std::string m_id;
		int64_t m_date;
		std::string m_author;
		std::string m_message;
		Diffstat m_diffstat;
};


#endif // REVISION_H_
