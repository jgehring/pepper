/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: subversion.cpp
 * Subversion repository backend
 */


#include "backends/subversion.h"


// Constructor
SubversionBackend::SubversionBackend(const Options &options)
	: Backend(options)
{

}

// Destructor
SubversionBackend::~SubversionBackend()
{

}

// Returns the revision data for the given ID
Revision *SubversionBackend::revision(const std::string &id)
{
	// TODO
	return NULL;
}

// Returns the HEAD revision for the current branch
std::string SubversionBackend::head(const std::string &branch)
{
	return "HEAD";
}

// Returns a list of available branches
std::vector<std::string> SubversionBackend::branches()
{
	std::vector<std::string> branches;
	branches.push_back("trunk");
	return branches;
}
