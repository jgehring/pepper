/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: repository.cpp
 * Repository interface for report scripts
 */


#include "backend.h"
#include "options.h"

#include "repository.h"


// Constructor
Repository::Repository(Backend *backend)
	: m_backend(backend)
{

}

// Destructor
Repository::~Repository()
{

}

// Returns the backend
Backend *Repository::backend() const
{
	return m_backend;
}
