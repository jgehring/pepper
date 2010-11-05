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

// Returns a unique identifier for this repository
std::string GitBackend::uuid()
{
	return std::string();
}

// Returns the HEAD revision for the current branch
std::string GitBackend::head(const std::string &branch)
{
	return "HEAD";
}

// Returns the standard branch (i.e., master)
std::string GitBackend::mainBranch()
{
	return "master";
}

// Returns a list of available branches
std::vector<std::string> GitBackend::branches()
{
	std::vector<std::string> branches;
	branches.push_back("master");
	return branches;
}

// Returns a diffstat for the specified revision
Diffstat GitBackend::diffstat(const std::string &id)
{
	// TODO
	return Diffstat();
}

// Returns a revision iterator for the given branch
Backend::RevisionIterator *GitBackend::iterator(const std::string &branch)
{
	RevisionIterator *it = new RevisionIterator();
	// TODO: Set branches
	return it;
}

// Returns the revision data for the given ID
Revision *GitBackend::revision(const std::string &id)
{
	return NULL;
}
