/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: backend.h
 * Abstract repository backend (interface)
 */


#ifndef BACKEND_H_
#define BACKEND_H_


class Options;


class Backend
{
	public:
		static Backend *backendFor(const Options &options);

	protected:
		Backend(const std::string &url);
};


#endif // BACKEND_H_
