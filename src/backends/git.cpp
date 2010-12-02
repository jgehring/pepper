/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: git.cpp
 * Git repository backend
 */


#include <algorithm>
#include <sstream>

#include "jobqueue.h"
#include "options.h"
#include "revision.h"
#include "utils.h"

#include "syslib/fs.h"
#include "syslib/parallel.h"

#include "backends/git.h"


// Diffstat fetching worker thread
class GitDiffstatThread : public sys::parallel::Thread
{
public:
	GitDiffstatThread(JobQueue<std::string, Diffstat> *queue)
		: m_queue(queue)
	{
	}

protected:
	void run()
	{
		std::string revision;
		while (m_queue->getArg(&revision)) {
			int ret;
			std::string out = utils::exec(std::string("git diff-tree -U0 -a -m --no-renames --root ")+revision, &ret);
			if (ret != 0) {
				m_queue->failed(revision);
				continue;
			}
			std::istringstream in(out);
			m_queue->done(revision, DiffParser::parse(in));
		}
	}

private:
	JobQueue<std::string, Diffstat> *m_queue;
};


// Handles the prefetching of diffstats
class GitDiffstatPrefetcher
{
public:
	GitDiffstatPrefetcher(int n = 4)
	{
		for (int i = 0; i < n; i++) {
			GitDiffstatThread * thread = new GitDiffstatThread(&m_queue);
			thread->start();
			m_threads.push_back(thread);
		}
	}

	~GitDiffstatPrefetcher()
	{
		for (unsigned int i = 0; i < m_threads.size(); i++) {
			delete m_threads[i];
		}
	}

	void stop()
	{
		m_queue.stop();
	}

	void wait()
	{
		for (unsigned int i = 0; i < m_threads.size(); i++) {
			m_threads[i]->wait();
		}
	}

	void prefetch(const std::vector<std::string> &revisions)
	{
		m_queue.put(revisions);
	}

	bool get(const std::string &revision, Diffstat *dest)
	{
		return m_queue.getResult(revision, dest);
	}

	bool willFetch(const std::string &revision)
	{
		return m_queue.hasArg(revision);
	}

private:
	JobQueue<std::string, Diffstat> m_queue;
	std::vector<GitDiffstatThread *> m_threads;
};


// Constructor
GitBackend::GitBackend(const Options &options)
	: Backend(options), m_prefetcher(NULL)
{

}

// Destructor
GitBackend::~GitBackend()
{
	if (m_prefetcher) {
		m_prefetcher->stop();
		m_prefetcher->wait();
		delete m_prefetcher;
	}
}

// Initializes the backend
void GitBackend::init()
{
	std::string repo = m_opts.repoUrl();
	if (sys::fs::exists(repo + "/HEAD")) {
		setenv("GIT_DIR", repo.c_str(), 1);
	} else if (sys::fs::exists(repo + "/.git/HEAD")) {
		setenv("GIT_DIR", (repo + "/.git").c_str(), 1);
	} else {
		throw PEX(utils::strprintf("Not a git repository: %s", repo.c_str()));
	}
}

// Returns true if this backend is able to access the given repository
bool GitBackend::handles(const std::string &url)
{
	if (sys::fs::dirExists(url+"/.git")) {
		return true;
	} else if (sys::fs::dirExists(url) && !url.compare(url.length() - 5, 5, "/.git")) {
		// Bare repository
		return true;
	}
	return false;
}

// Returns a unique identifier for this repository
std::string GitBackend::uuid()
{
	// Use the SHA1 of the first commit of the master branch as the UUID.
	// Let's try to find the "master" branch
	int ret;
	std::string out = utils::exec("git branch -a", &ret);
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retreive the list of branches (%d)", ret));
	}
	std::vector<std::string> branches = utils::split(out, "\n");
	std::string branch;
	for (unsigned int i = 0; i < branches.size(); i++) {
		if (branches[i].empty()) {
			continue;
		}
		if (branches[i][0] == '*') {
			branch = branches[i]; // Fallback branch: current one
		}
		branches[i] = branches[i].substr(2);
	}

	if (std::search_n(branches.begin(), branches.end(), 1, "master") != branches.end()) {
		branch = "master";
	}
	if (std::search_n(branches.begin(), branches.end(), 1, "remotes/origin/master") != branches.end()) {
		branch = "remotes/origin/master";
	}

	if (branch.empty()) {
		return std::string();
	}

	// Get ID of first commit of the root branch
	std::string id = utils::exec(std::string("git rev-list --reverse -1 ")+branch, &ret);
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to determine the root commit for branch '%s' (%d)", branch.c_str(), ret));
	}
	return utils::trim(id);
}

// Returns the HEAD revision for the current branch
std::string GitBackend::head(const std::string &branch)
{
	return "HEAD";
}

// Returns the currently checked out branch
std::string GitBackend::mainBranch()
{
	int ret;
	std::string out = utils::exec("git branch", &ret);
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retreive the list of branches (%d)", ret));
	}
	std::vector<std::string> branches = utils::split(out, "\n");
	for (unsigned int i = 0; i < branches.size(); i++) {
		if (!branches[i].empty() && branches[i][0] == '*') {
			return branches[i].substr(2);
		}
	}
	return "master";
}

// Returns a list of available branches
std::vector<std::string> GitBackend::branches()
{
	int ret;
	std::string out = utils::exec("git branch", &ret);
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retreive the list of branches (%d)", ret));
	}
	std::vector<std::string> branches = utils::split(out, "\n");
	for (unsigned int i = 0; i < branches.size(); i++) {
		if (branches[i].empty()) {
			branches.erase(branches.begin()+i);
			--i;
			continue;
		}
		branches[i] = branches[i].substr(2);
	}
	return branches;
}

// Returns a diffstat for the specified revision
Diffstat GitBackend::diffstat(const std::string &id)
{
	// Maybe it's prefetched
	if (m_prefetcher && m_prefetcher->willFetch(id)) {
		Diffstat stat;
		if (!m_prefetcher->get(id, &stat)) {
			throw PEX(utils::strprintf("Failed to retrieve diffstat for revision %s", id.c_str()));
		}
		return stat;
	}

	int ret;
	std::string out = utils::exec(std::string("git diff-tree -U0 -a -m --no-renames --root ")+id, &ret);
	if (ret != 0) {
		throw PEX(utils::strprintf("Failed to retrieve diffstat for revision %s (%d)", id.c_str(), ret));
	}
	std::istringstream in(out);
	return DiffParser::parse(in);
}

// Returns a revision iterator for the given branch
Backend::LogIterator *GitBackend::iterator(const std::string &branch)
{
	int ret;
	std::string out = utils::exec(std::string("git rev-list ")+branch, &ret);
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retreive log for branch '%s' (%d)", branch.c_str(), ret));
	}
	std::vector<std::string> revisions = utils::split(out, "\n");
	while (!revisions.empty() && revisions[revisions.size()-1].empty()) {
		revisions.pop_back();
	}
	std::reverse(revisions.begin(), revisions.end());
	return new LogIterator(revisions);
}

// Starts prefetching the given revision IDs
void GitBackend::prefetch(const std::vector<std::string> &ids)
{
	if (m_prefetcher == NULL) {
		m_prefetcher = new GitDiffstatPrefetcher();
	}
	m_prefetcher->prefetch(ids);
}

// Returns the revision data for the given ID
Revision *GitBackend::revision(const std::string &id)
{
	int ret;
	std::string meta = utils::exec(std::string("git log -1 --pretty=format:\"%ct\n%aN\n%B\" ")+id, &ret);
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retreive meta-data for revision '%s' (%d)", id.c_str(), ret));
	}
	std::vector<std::string> lines = utils::split(meta, "\n");
	int64_t date;
	std::string author;
	if (!lines.empty()) {
		utils::str2int(lines[0], &date);
		lines.erase(lines.begin());
	}
	if (!lines.empty()) {
		author = lines[0];
		lines.erase(lines.begin());
	}
	std::string msg = utils::join(lines, "\n");
	return new Revision(id, date, author, msg, diffstat(id));
}

// Handle cleanup of diffstat scheduler
void GitBackend::finalize()
{
	if (m_prefetcher) {
		m_prefetcher->stop();
		m_prefetcher->wait();
		delete m_prefetcher;
		m_prefetcher = NULL;
	}
}
