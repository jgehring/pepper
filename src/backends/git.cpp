/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: git.cpp
 * Git repository backend
 */


#include "backends/git.h"


// Constructor
GitBackend::GitBackend(const Options &options)
	: Backend(options)
{

}

// Destructor
GitBackend::~GitBackend()
{

}

// Returns the revision data for the given ID
Revision *GitBackend::revision(const std::string &id)
{
	return NULL;
}

// Returns the HEAD revision for the current branch
std::string GitBackend::head(const std::string &branch)
{
	return "HEAD";
}

// Returns a list of available branches
std::vector<std::string> GitBackend::branches()
{
	std::vector<std::string> branches;
	branches.push_back("master");
	return branches;
}
