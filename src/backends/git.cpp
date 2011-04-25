/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: git.cpp
 * Git repository backend
 */


#include "main.h"

#include <algorithm>
#include <fstream>

#include "jobqueue.h"
#include "logger.h"
#include "options.h"
#include "revision.h"
#include "utils.h"

#include "syslib/fs.h"
#include "syslib/io.h"
#include "syslib/parallel.h"

#include "backends/git.h"


// Diffstat fetching worker thread, using a pipe to write data to "git diff-tree"
class GitDiffstatPipe : public sys::parallel::Thread
{
public:
	GitDiffstatPipe(const std::string &git, JobQueue<std::string, Diffstat> *queue)
		: m_git(git), m_queue(queue)
	{
	}

	static Diffstat diffstat(const std::string &git, const std::string &id, const std::string &parent = std::string())
	{
		if (!parent.empty()) {
			sys::io::PopenStreambuf buf(git.c_str(), "diff-tree", "-U0", "--no-renames", parent.c_str(), id.c_str());
			std::istream in(&buf);
			Diffstat stat = DiffParser::parse(in);
			if (buf.close() != 0) {
				throw PEX("git diff-tree command failed");
			}
			return stat;
		} else {
			sys::io::PopenStreambuf buf(git.c_str(), "diff-tree", "-U0", "--no-renames", "--root", id.c_str());
			std::istream in(&buf);
			Diffstat stat = DiffParser::parse(in);
			if (buf.close() != 0) {
				throw PEX("git diff-tree command failed");
			}
			return stat;
		}
	}

protected:
	void run()
	{
		// TODO: Error checking
		sys::io::PopenStreambuf buf(m_git.c_str(), "diff-tree", "-U0", "--no-renames", "--stdin", "--root", NULL, NULL, std::ios::in | std::ios::out);
		std::istream in(&buf);
		std::ostream out(&buf);

		std::string revision;
		while (m_queue->getArg(&revision)) {
			std::vector<std::string> revs = utils::split(revision, ":");

			if (revs.size() < 2) {
				out << revs[0] << '\n';
			} else {
				out << revs[1] << " " << revs[0] << '\n';
			}

			// We use EOF characters to mark the end of a revision for
			// the diff parser. git diff-tree won't understand this line
			// and simply write the EOF.
			out << (char)EOF << '\n' << std::flush;

			Diffstat stat = DiffParser::parse(in);
			m_queue->done(revision, stat);
		}
	}

private:
	std::string m_git;
	JobQueue<std::string, Diffstat> *m_queue;
};


// Handles the prefetching of diffstats
class GitDiffstatPrefetcher
{
public:
	GitDiffstatPrefetcher(const std::string &git, int n = -1)
	{
		if (n < 0) {
			n = std::max(1, sys::parallel::idealThreadCount() / 2);
		}
		Logger::info() << "GitBackend: Using " << n << " threads for prefetching diffstats" << endl;
		for (int i = 0; i < n; i++) {
			sys::parallel::Thread *thread = new GitDiffstatPipe(git, &m_queue);
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
	std::vector<sys::parallel::Thread *> m_threads;
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
	std::string repo = m_opts.repository();
	if (sys::fs::exists(repo + "/HEAD")) {
		setenv("GIT_DIR", repo.c_str(), 1);
	} else if (sys::fs::exists(repo + "/.git/HEAD")) {
		setenv("GIT_DIR", (repo + "/.git").c_str(), 1);
	} else if (sys::fs::fileExists(repo + "/.git")) {
		PDEBUG << "Parsing .git file" << endl;
		std::ifstream in((repo + "/.git").c_str(), std::ios::in);
		if (!in.good()) {
			throw PEX(utils::strprintf("Unable to read from .git file: %s", repo.c_str()));
		}
		std::string str;
		std::getline(in, str);
		std::vector<std::string> parts = utils::split(str, ":");
		if (parts.size() < 2) {
			throw PEX(utils::strprintf("Unable to parse contents of .git file: %s", str.c_str()));
		}
		setenv("GIT_DIR", utils::trim(parts[1]).c_str(), 1);
	} else {
		throw PEX(utils::strprintf("Not a git repository: %s", repo.c_str()));
	}

	// Search for git executable
	char *path = getenv("PATH");
	if (path == NULL) {
		throw PEX("PATH is not set");
	}
	std::vector<std::string> ls;
#ifdef POS_WIN
	ls = utils::split(path, ";");
#else
	ls = utils::split(path, ":");
#endif
	for (size_t i = 0; i < ls.size(); i++) {
		std::string t = ls[i] + "/git";
#ifdef POS_WIN
		t += ".exe";
#endif
		if (sys::fs::fileExecutable(t)) {
			m_git = t;
		}
	}

	if (m_git.empty()) {
		throw PEX("Can't find git in PATH");
	}

	PDEBUG << "git executable is " << m_git << endl;
	PDEBUG << "GIT_DIR has been set to " << getenv("GIT_DIR") << endl;
}

// Returns true if this backend is able to access the given repository
bool GitBackend::handles(const std::string &url)
{
	if (sys::fs::dirExists(url+"/.git")) {
		return true;
	} else if (sys::fs::fileExists(url+"/.git")) {
		PDEBUG << "Detached repository detected" << endl;
		return true;
	} else if (sys::fs::dirExists(url) && sys::fs::fileExists(url+"/HEAD") && sys::fs::dirExists(url+"/objects")) {
		PDEBUG << "Bare repository detected" << endl;
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
	std::string out = sys::io::exec(&ret, m_git.c_str(), "branch", "-a");
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retrieve the list of branches (%d)", ret));
	}
	std::vector<std::string> branches = utils::split(out, "\n");
	std::string branch;
	for (unsigned int i = 0; i < branches.size(); i++) {
		if (branches[i].empty()) {
			continue;
		}
		if (branches[i][0] == '*') {
			branch = branches[i].substr(2); // Fallback branch: current one
		}
		branches[i] = branches[i].substr(2);
	}

	if (branch.empty()) {
		if (std::search_n(branches.begin(), branches.end(), 1, "master") != branches.end()) {
			branch = "master";
		} else if (std::search_n(branches.begin(), branches.end(), 1, "remotes/origin/master") != branches.end()) {
			branch = "remotes/origin/master";
		}
	}
	if (branch.empty()) {
		throw PEX("Unable to retrieve UUID for repository");
	}

	// Get ID of first commit of the root branch
	// Unfortunatley, the --max-count=n option results in n revisions counting from the HEAD.
	// This way, we'll always get the HEAD revision with --max-count=1.
	std::string id = sys::io::exec(&ret, m_git.c_str(), "rev-list", "--reverse", branch.c_str(), "--");
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to determine the root commit for branch '%s' (%d)", branch.c_str(), ret));
	}
	size_t pos = id.find_first_of('\n');
	if (pos == std::string::npos) {
		throw PEX(utils::strprintf("Unable to determine the root commit for branch '%s' (%d)", branch.c_str(), ret));
	}
	return utils::trim(id.substr(0, pos));
}

// Returns the HEAD revision for the given branch
std::string GitBackend::head(const std::string &branch)
{
	int ret;
	std::string out = sys::io::exec(&ret, m_git.c_str(), "rev-list", "-1", (branch.empty() ? "HEAD" : branch).c_str(), "--");
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retrieve head commit for branch %s (%d)", branch.c_str(), ret));
	}
	return utils::trim(out);
}

// Returns the currently checked out branch
std::string GitBackend::mainBranch()
{
	int ret;
	std::string out = sys::io::exec(&ret, m_git.c_str(), "branch");
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retrieve the list of branches (%d)", ret));
	}
	std::vector<std::string> branches = utils::split(out, "\n");
	for (unsigned int i = 0; i < branches.size(); i++) {
		if (!branches[i].empty() && branches[i][0] == '*') {
			return branches[i].substr(2);
		}
	}
	return "master";
}

// Returns a list of available local branches
std::vector<std::string> GitBackend::branches()
{
	int ret;
	std::string out = sys::io::exec(&ret, m_git.c_str(), "branch");
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retrieve the list of branches (%d)", ret));
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

// Returns a list of available tags
std::vector<Tag> GitBackend::tags()
{
	int ret;

	// Fetch list of tag names
	std::string out = sys::io::exec(&ret, m_git.c_str(), "tag");
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retrieve the list of tags (%d)", ret));
	}
	std::vector<std::string> names = utils::split(out, "\n");
	std::vector<Tag> tags;

	// Determine corresponding commits
	for (unsigned int i = 0; i < names.size(); i++) {
		if (names[i].empty()) {
			continue;
		}

		std::string out = sys::io::exec(&ret, m_git.c_str(), "rev-list", "-1", names[i].c_str());
		if (ret != 0) {
			throw PEX(utils::strprintf("Unable to retrieve the list of tags (%d)", ret));
		}

		std::string id = utils::trim(out);
		if (!id.empty()) {
			tags.push_back(Tag(id, names[i]));
		}
	}
	return tags;
}

// Returns a diffstat for the specified revision
Diffstat GitBackend::diffstat(const std::string &id)
{
	// Maybe it's prefetched
	if (m_prefetcher && m_prefetcher->willFetch(id)) {
		PDEBUG << "Revision " << id << " will be prefetched" << endl;
		Diffstat stat;
		if (!m_prefetcher->get(id, &stat)) {
			throw PEX(utils::strprintf("Failed to retrieve diffstat for revision %s", id.c_str()));
		}
		return stat;
	}

	PDEBUG << "Fetching revision " << id << " manually" << endl;

	std::vector<std::string> revs = utils::split(id, ":");
	if (revs.size() > 1) {
		return GitDiffstatPipe::diffstat(m_git, revs[1], revs[0]);
	}
	return GitDiffstatPipe::diffstat(m_git, revs[0]);
}

// Returns a file listing for the given revision (defaults to HEAD)
std::vector<std::string> GitBackend::tree(const std::string &id)
{
	int ret;
	std::string out = sys::io::exec(&ret, m_git.c_str(), "ls-tree", "-r", "--full-name", "--name-only", (id.empty() ? "HEAD" : id.c_str()));
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retrieve tree listing for ID '%s' (%d)", id.c_str(), ret));
	}
	std::vector<std::string> contents = utils::split(out, "\n");
	while (!contents.empty() && contents[contents.size()-1].empty()) {
		contents.pop_back();
	}
	return contents;
}

// Returns a revision iterator for the given branch
Backend::LogIterator *GitBackend::iterator(const std::string &branch, int64_t start, int64_t end)
{
	int ret;
	std::string out;
	if (start >= 0) {
		std::string maxage = utils::strprintf("--max-age=%lld", start);
		if (end >= 0) {
			std::string minage = utils::strprintf("--min-age=%lld", end);
			out = sys::io::exec(&ret, m_git.c_str(), "rev-list", "--first-parent", "--reverse", maxage.c_str(), minage.c_str(), branch.c_str(), "--");
		} else {
			out = sys::io::exec(&ret, m_git.c_str(), "rev-list", "--first-parent", "--reverse", maxage.c_str(), branch.c_str(), "--");
		}
	} else {
		if (end >= 0) {
			std::string minage = utils::strprintf("--min-age=%lld", end);
			out = sys::io::exec(&ret, m_git.c_str(), "rev-list", "--first-parent", "--reverse", minage.c_str(), branch.c_str(), "--");
		} else {
			out = sys::io::exec(&ret, m_git.c_str(), "rev-list", "--first-parent", "--reverse", branch.c_str(), "--");
		}
	}
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retrieve log for branch '%s' (%d)", branch.c_str(), ret));
	}
	std::vector<std::string> revisions = utils::split(out, "\n");
	while (!revisions.empty() && revisions[revisions.size()-1].empty()) {
		revisions.pop_back();
	}

	// Add parent revisions, so diffstat fetching will give correct results
	for (ssize_t i = revisions.size()-1; i > 0; i--) {
		revisions[i] = revisions[i-1] + ":" + revisions[i];
	}

	return new LogIterator(revisions);
}

// Starts prefetching the given revision IDs
void GitBackend::prefetch(const std::vector<std::string> &ids)
{
	if (m_prefetcher == NULL) {
		m_prefetcher = new GitDiffstatPrefetcher(m_git);
	}
	m_prefetcher->prefetch(ids);
	PDEBUG << "Started prefetching " << ids.size() << " revisions" << endl;
}

// Returns the revision data for the given ID
Revision *GitBackend::revision(const std::string &id)
{
	// Unfortunately, older git versions don't have the %B format specifier
	// for unwrapped subject and body, so the raw commit headers will be parsed instead.
#if 0
	int ret;
	std::string meta = sys::io::exec(&ret, m_git.c_str(), "log", "-1", "--pretty=format:%ct\n%aN\n%B", id.c_str());
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retrieve meta-data for revision '%s' (%d, %s)", id.c_str(), ret, meta.c_str()));
	}
	std::vector<std::string> lines = utils::split(meta, "\n");
	int64_t date = 0;
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
#else

	std::string rev = utils::split(id, ":").back();

	int ret;
	std::string header = sys::io::exec(&ret, m_git.c_str(), "rev-list", "-1", "--header", rev.c_str());
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retrieve meta-data for revision '%s' (%d, %s)", rev.c_str(), ret, header.c_str()));
	}
	std::vector<std::string> lines = utils::split(header, "\n");
	if (lines.size() < 6) {
		throw PEX(utils::strprintf("Unable to parse meta-data for revision '%s' (%d, %s)", rev.c_str(), ret, header.c_str()));
	}

	// Parse author information
	unsigned int i = 0;
	while (i < lines.size() && lines[i].compare(0, 7, "author ")) {
		++i;
	}
	if (i >= lines.size()) {
		throw PEX(utils::strprintf("Unable to parse author information for revision '%s' (%d, %s)", rev.c_str(), ret, header.c_str()));
	}
	std::vector<std::string> authorln = utils::split(lines[i], " ");
	if (authorln.size() < 4) {
		throw PEX(utils::strprintf("Unable to parse author information for revision '%s' (%d, %s)", rev.c_str(), ret, lines[i].c_str()));
	}

	// Author: 2nd to n-2nd entry
	std::string author = utils::join(authorln.begin()+1, authorln.end()-2, " ");
	// Strip email address, assuming a start at the last "<" (not really compliant with RFC2882)
	author = utils::trim(author.substr(0, author.find_last_of('<')));

	// Committer ate: last 2 entries in the form %s %z
	while (i < lines.size() && lines[i].compare(0, 10, "committer ")) {
		++i;
	}
	if (i >= lines.size()) {
		throw PEX(utils::strprintf("Unable to parse date information for revision '%s' (%d, %s)", rev.c_str(), ret, header.c_str()));
	}
	std::vector<std::string> dateln = utils::split(lines[i], " ");
	if (dateln.size() < 2) {
		throw PEX(utils::strprintf("Unable to parse date information for revision '%s' (%d, %s)", rev.c_str(), ret, lines[i].c_str()));
	}
	int64_t date = 0, off = 0;
	if (!utils::str2int(dateln[dateln.size()-2], &date, 10) || !utils::str2int(dateln[dateln.size()-1], &off, 10)) {
		throw PEX(utils::strprintf("Unable to parse date information for revision '%s' (%d, %s)", rev.c_str(), ret, lines[i].c_str()));
	}
	date += off;

	// Last but not least: commit message
	while (i < lines.size() && !lines[i].empty()) {
		++i;
	}
	std::string msg;
	++i;
	while (i < lines.size()) {
		if (lines[i].length() > 4) {
			msg += lines[i].substr(4);
		}
		if (i < lines.size()-1) {
			msg += "\n";
		}
		++i;
	}
#endif

	return new Revision(id, date, author, msg, diffstat(id));
}

// Handle cleanup of diffstat scheduler
void GitBackend::finalize()
{
	if (m_prefetcher) {
		PDEBUG << "Waiting for prefetcher... " << endl;
		m_prefetcher->stop();
		m_prefetcher->wait();
		delete m_prefetcher;
		m_prefetcher = NULL;
		PDEBUG << "done" << endl;
	}
}
