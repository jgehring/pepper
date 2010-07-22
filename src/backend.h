/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: backend.h
 * Abstract repository backend (interface)
 */


#ifndef BACKEND_H_
#define BACKEND_H_


#include <string>

class Options;
class Revision;


class Backend
{
	public:
		virtual ~Backend();

		static Backend *backendFor(const Options &options);

		virtual Revision *revision(const std::string &id) = 0;

	protected:
		Backend(const Options &options);

	protected:
		const Options &m_opts;
};


#endif // BACKEND_H_
