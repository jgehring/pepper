/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: mercurial.cpp
 * Mercurial repository backend
 */


#include <algorithm>
#include <sstream>

#include "options.h"
#include "revision.h"
#include "sys/fs.h"

#include "utils.h"

#include "backends/mercurial.h"


// Constructor
MercurialBackend::MercurialBackend(const Options &options)
	: Backend(options)
{

}

// Destructor
MercurialBackend::~MercurialBackend()
{

}

// Initializes the backend
void MercurialBackend::init()
{
	std::string repo = m_opts.repoUrl();
	if (!sys::fs::dirExists(repo + "/.hg")) {
		throw PEX(utils::strprintf("Not a mercurial repository: %s", repo.c_str()));
	}
}

// Returns a unique identifier for this repository
std::string MercurialBackend::uuid()
{
	// TODO
	return utils::join(utils::split(m_opts.repoUrl(), "/"), "_");
}

// Returns the HEAD revision for the current branch
std::string MercurialBackend::head(const std::string &branch)
{
	return "HEAD";
}

// Returns the standard branch (i.e., default)
std::string MercurialBackend::mainBranch()
{
	return "default";
}

// Returns a list of available branches
std::vector<std::string> MercurialBackend::branches()
{
	std::string out = utils::exec(hgcmd()+" branch");
	std::vector<std::string> branches = utils::split(out, "\n");
	for (unsigned int i = 0; i < branches.size(); i++) {
		branches[i] = branches[i].substr(2);
	}
	return branches;
}

// Returns a diffstat for the specified revision
Diffstat MercurialBackend::diffstat(const std::string &id)
{
	std::string out = utils::exec(hgcmd()+" diff --git --change "+id);
	std::istringstream in(out);
	return DiffParser::parse(in);
}

// Returns a revision iterator for the given branch
Backend::LogIterator *MercurialBackend::iterator(const std::string &branch)
{
	std::string out = utils::exec(hgcmd()+" log --quiet --branch "+branch);
	std::vector<std::string> revisions = utils::split(out, "\n");
	if (!revisions.empty()) {
		revisions.pop_back();
	}
	for (unsigned int i = 0; i < revisions.size(); i++) {
		size_t pos = revisions[i].find(':');
		if (pos != std::string::npos) {
			revisions[i] = revisions[i].substr(pos+1);
		}
	}
	std::reverse(revisions.begin(), revisions.end());
	return new LogIterator(revisions);
}

// Returns the revision data for the given ID
Revision *MercurialBackend::revision(const std::string &id)
{
	std::string meta = utils::exec(hgcmd()+" log -r "+id+" --template=\"{date|hgdate}\n{author|person}\n{desc}\"");
	std::vector<std::string> lines = utils::split(meta, "\n");
	int64_t date;
	std::string author;
	if (!lines.empty()) {
		// Date is given as seconds and timezone offset from UTC
		std::vector<std::string> parts = utils::split(lines[0], " ");
		if (parts.size() > 1) {
			int64_t offset;
			utils::str2int(parts[0], &date);
			utils::str2int(parts[1], &offset);
			date += offset;
		}
		lines.erase(lines.begin());
	}
	if (!lines.empty()) {
		author = lines[0];
		lines.erase(lines.begin());
	}
	std::string msg = utils::join(lines, "\n");
	return new Revision(id, date, author, msg, diffstat(id));
}

// Returns the hg command with the correct --repository command line switch
std::string MercurialBackend::hgcmd() const
{
	return std::string("hg --noninteractive --repository ") + m_opts.repoUrl();
}
