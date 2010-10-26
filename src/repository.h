/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: repository.h
 * Repository interface (interface)
 */


#ifndef REPOSITORY_H_
#define REPOSITORY_H_


class Backend;


class Repository
{
	friend class LuaRepository;

	public:
		Repository(Backend *backend);
		~Repository();

		Backend *backend() const;

	private:
		Backend *m_backend;
};


#endif // REPOSITORY_H_
