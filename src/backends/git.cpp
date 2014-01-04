/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2014 Jonas Gehring
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

#include <unistd.h>

#include "jobqueue.h"
#include "logger.h"
#include "options.h"
#include "revision.h"
#include "strlib.h"
#include "utils.h"

#include "syslib/datetime.h"
#include "syslib/fs.h"
#include "syslib/io.h"
#include "syslib/parallel.h"

#include "backends/git.h"


// Diffstat fetching worker thread, using a pipe to write data to "git diff-tree"
class GitDiffstatPipe : public sys::parallel::Thread
{
public:
	GitDiffstatPipe(const std::string &gitpath, JobQueue<std::string, Diffstat> *queue)
		: m_gitpath(gitpath), m_queue(queue)
	{
	}

	static Diffstat diffstat(const std::string &gitpath, const std::string &id, const std::string &parent = std::string())
	{
		if (!parent.empty()) {
			sys::io::PopenStreambuf buf((gitpath+"/git-diff-tree").c_str(), "-U0", "--no-renames", parent.c_str(), id.c_str());
			std::istream in(&buf);
			Diffstat stat = DiffParser::parse(in);
			if (buf.close() != 0) {
				throw PEX("git diff-tree command failed");
			}
			return stat;
		} else {
			sys::io::PopenStreambuf buf((gitpath+"/git-diff-tree").c_str(), "-U0", "--no-renames", "--root", id.c_str());
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
		sys::io::PopenStreambuf buf((m_gitpath+"/git-diff-tree").c_str(), "-U0", "--no-renames", "--stdin", "--root", NULL, NULL, NULL, std::ios::in | std::ios::out);
		std::istream in(&buf);
		std::ostream out(&buf);

		std::string revision;
		while (m_queue->getArg(&revision)) {
			std::vector<std::string> revs = str::split(revision, ":");

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
	std::string m_gitpath;
	JobQueue<std::string, Diffstat> *m_queue;
};


// Meta-data fetching worker thread, passing multiple revisions to git-rev-list at once.
// Note that all IDs coming from the JobQueue are expected to contain a single hash,
// i.e. no parent:child ID spec.
class GitMetaDataThread : public sys::parallel::Thread
{
public:
	struct Data
	{
		int64_t date;
		std::string author;
		std::string message;
	};

public:
	GitMetaDataThread(const std::string &gitpath, JobQueue<std::string, Data> *queue)
		: m_gitpath(gitpath), m_queue(queue)
	{
	}

	static void parseHeader(const std::vector<std::string> &header, Data *dest)
	{
		// TODO: Proper exception descriptions
		if (header.size() < 5) {
			throw PEX(str::printf("Unable to parse meta-data"));
		}

		// Here's the raw header format. The 'parent' line is not present for
		// non-root commits.
		// $ID_HASH
		// tree $TREE_HASH
		// parent $PARENT_HASH
		// author $AUTHOR_NAME $AUTHOR_EMAIL $DATE $OFFSET
		// committer $AUTHOR_NAME $AUTHOR_EMAIL $DATE $OFFSET
		//
		//     $MESSAGE_INDENTED_BY_4_SPACES

		// Find author line
		size_t line = 0;
		while (line < header.size() && header[line].compare(0, 7, "author ")) {
			++line;
		}
		if (line >= header.size()) {
			PDEBUG << "Invalid header:" << endl;
			for (size_t i = 0; i < header.size(); i++) {
				PDEBUG << header[i] << endl;
			}
			throw PEX(str::printf("Unable to parse meta-data"));
		}

		// Author information
		dest->author = header[line].substr(7);

		// Strip email address and date, assuming a start at the last "<" (not really compliant with RFC2882)
		size_t pos = dest->author.find_last_of('<');
		if (pos != std::string::npos) {
			dest->author = dest->author.substr(0, pos);
		}
		dest->author = str::trim(dest->author);

		// Commiter date
		if (header[++line].compare(0, 10, "committer ")) {
			throw PEX(str::printf("Unable to parse commit date from line: %s", header[line].c_str()));
		}
		if ((pos = header[line].find_last_of(' ')) == std::string::npos) {
			throw PEX(str::printf("Unable to parse commit date from line: %s", header[line].c_str()));
		}
		size_t pos2 = header[line].find_last_of(' ', pos - 1);
		if (pos2 == std::string::npos || !str::str2int(header[line].substr(pos2, pos - pos2), &(dest->date), 10)) {
			throw PEX(str::printf("Unable to parse commit date from line: %s", header[line].c_str()));
		}
		int64_t offset_hr = 0, offset_min = 0;
		if (!str::str2int(header[line].substr(pos+1, 3), &offset_hr, 10) || !str::str2int(header[line].substr(pos+4, 2), &offset_min, 10)) {
			throw PEX(str::printf("Unable to parse commit date from line: %s", header[line].c_str()));
		}
		dest->date += offset_hr * 60 * 60 + offset_min * 60;

		// Commit message
		dest->message.clear();
		for (size_t i = line+1; i < header.size(); i++) {
			if (header[i].length() > 4) {
				dest->message += header[i].substr(4);
			}
			if (!header[i].empty() && header[i][0] != '\0') {
				dest->message += "\n";
			}
		}
	}

	static void metaData(const std::string &gitpath, const std::string &id, Data *dest)
	{
		int ret;
		std::string header = sys::io::exec(&ret, (gitpath+"/git-rev-list").c_str(), "-1", "--header", id.c_str());
		if (ret != 0) {
			throw PEX(str::printf("Unable to retrieve meta-data for revision '%s' (%d, %s)", id.c_str(), ret, header.c_str()));
		}

		parseHeader(str::split(header, "\n"), dest);
	}

protected:
	void run()
	{
		Data data;
		const size_t maxids = 64;
		const char **args = new const char *[maxids + 4];
		std::vector<std::string> ids;

		std::string revlist = m_gitpath + "/git-rev-list";
		args[0] = "--no-walk";
		args[1] = "--header"; // No support for message bodies in old git versions

		// Try to fetch the revision headers for maxrevs revisions at once
		while (m_queue->getArgs(&ids, maxids)) {
			for (size_t i = 0; i < ids.size(); i++) {
				args[i+2] = ids[i].c_str();
			}
			args[ids.size()+2] = NULL;

			sys::io::PopenStreambuf buf(revlist.c_str(), args);
			std::istream in(&buf);

			// Parse headers
			std::string str;
			std::vector<std::string> header;
			while (in.good()) {
				std::getline(in, str);
				if (!str.empty() && str[0] == '\0') {
					try {
						parseHeader(header, &data);
						m_queue->done(header[0], data);
					} catch (const std::exception &ex) {
						PDEBUG << "Error parsing revision header: " << ex.what() << endl;
						m_queue->failed(header[0]);
					}

					header.clear();
					header.push_back(str.substr(1));
				} else {
					header.push_back(str);
				}
			}

			int ret = buf.close();
			if (ret != 0) {
				PDEBUG << "Error running rev-list, ret = " << ret << endl;

				// Try all IDs one by one, so the incorrect ones can be identified
				for (size_t i = 0; i < ids.size(); i++) {
					try {
						metaData(m_gitpath, ids[i], &data);
						m_queue->done(ids[i], data);
					} catch (const std::exception &ex) {
						PDEBUG << "Error parsing revision header: " << ex.what() << endl;
						m_queue->failed(ids[i]);
					}
				}
			}
		}

		delete[] args;
	}

private:
	std::string m_gitpath;
	JobQueue<std::string, Data> *m_queue;
};


// Handles the prefetching of revision meta-data and diffstats
class GitRevisionPrefetcher
{
public:
	GitRevisionPrefetcher(const std::string &git, int n = -1)
		: m_metaQueue(4096)
	{
		if (n < 0) {
			n = std::max(1, sys::parallel::idealThreadCount() / 2);
		}
		for (int i = 0; i < n; i++) {
			sys::parallel::Thread *thread = new GitDiffstatPipe(git, &m_diffQueue);
			thread->start();
			m_threads.push_back(thread);
		}

		// Limit to 4 threads to prevent meta queue congestions
		n = std::min(n, 4);
		for (int i = 0; i < n; i++) {
			sys::parallel::Thread *thread = new GitMetaDataThread(git, &m_metaQueue);
			thread->start();
			m_threads.push_back(thread);
		}

		Logger::info() << "GitBackend: Using " << m_threads.size() << " threads for prefetching diffstats ("
			<< m_threads.size()-n << ") / meta-data (" << n << ")" << endl;
	}

	~GitRevisionPrefetcher()
	{
		for (unsigned int i = 0; i < m_threads.size(); i++) {
			delete m_threads[i];
		}
	}

	void stop()
	{
		m_diffQueue.stop();
		m_metaQueue.stop();
	}

	void wait()
	{
		for (unsigned int i = 0; i < m_threads.size(); i++) {
			m_threads[i]->wait();
		}
	}

	void prefetch(const std::vector<std::string> &revisions)
	{
		m_diffQueue.put(revisions);

		// Put child commits only to the meta queue
		std::vector<std::string> children;
		children.resize(revisions.size());
		for (size_t i = 0; i < revisions.size(); i++) {
			children[i] = utils::childId(revisions[i]);
		}
		m_metaQueue.put(children);
	}

	bool getDiffstat(const std::string &revision, Diffstat *dest)
	{
		return m_diffQueue.getResult(revision, dest);
	}

	bool getMeta(const std::string &revision, GitMetaDataThread::Data *dest)
	{
		return m_metaQueue.getResult(utils::childId(revision), dest);
	}

	bool willFetchDiffstat(const std::string &revision)
	{
		return m_diffQueue.hasArg(revision);
	}

	bool willFetchMeta(const std::string &revision)
	{
		return m_metaQueue.hasArg(utils::childId(revision));
	}

private:
	JobQueue<std::string, Diffstat> m_diffQueue;
	JobQueue<std::string, GitMetaDataThread::Data> m_metaQueue;
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
	close();
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
			throw PEX(str::printf("Unable to read from .git file: %s", repo.c_str()));
		}
		std::string str;
		std::getline(in, str);
		std::vector<std::string> parts = str::split(str, ":");
		if (parts.size() < 2) {
			throw PEX(str::printf("Unable to parse contents of .git file: %s", str.c_str()));
		}
		setenv("GIT_DIR", str::trim(parts[1]).c_str(), 1);
	} else {
		throw PEX(str::printf("Not a git repository: %s", repo.c_str()));
	}

	// Search for git executable
	std::string git = sys::fs::which("git");;
	PDEBUG << "git executable is " << git << endl;

	int ret;
	m_gitpath = str::trim(sys::io::exec(&ret, git.c_str(), "--exec-path"));
	if (ret != 0) {
		throw PEX("Unable to determine git exec-path");
	}
	PDEBUG << "git exec-path is " << m_gitpath << endl;

	PDEBUG << "GIT_DIR has been set to " << getenv("GIT_DIR") << endl;
}

// Called after Report::run()
void GitBackend::close()
{
	// Clean up any prefetching threads
	finalize();
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
	// Determine current main branch and the HEAD revision
	std::string branch = mainBranch();
	std::string headrev = head(branch);
	std::string oldroot, oldhead;
	int ret;

	// The $GIT_DIR/pepper.cache file caches branch names and their root
	// commits. It consists of lines of the form
	// $BRANCH_NAME $HEAD $ROOT
	std::string cachefile = std::string(getenv("GIT_DIR")) + "/pepper.cache";
	{
		std::ifstream in((std::string(getenv("GIT_DIR")) + "/pepper.cache").c_str());
		while (in.good()) {
			std::string str;
			std::getline(in, str);
			if (str.compare(0, branch.length(), branch)) {
				continue;
			}

			std::vector<std::string> parts = str::split(str, " ");
			if (parts.size() == 3) {
				oldhead = parts[1];
				oldroot = parts[2];
				if (oldhead == headrev) {
					PDEBUG << "Found cached root commit" << endl;
					return oldroot;
				}
			}
			break;
		}
	}

	// Check if the old root commit is still valid by checking if the old head revision
	// is an ancestory of the current one
	std::string root;
	if (!oldroot.empty()) {
		std::string ref = sys::io::exec(&ret, (m_gitpath+"/git-rev-list").c_str(), "-1", (oldhead + ".." + headrev).c_str());
		if (ret == 0 && !ref.empty()) {
			PDEBUG << "Old head " << oldhead << " is a valid ancestor, updating cached head" << endl;
			root = oldroot;
		}
	}

	// Get ID of first commit of the selected branch
	// Unfortunatley, the --max-count=n option results in n revisions counting from the HEAD.
	// This way, we'll always get the HEAD revision with --max-count=1.
	if (root.empty()) {
		sys::datetime::Watch watch;

		std::string id = sys::io::exec(&ret, (m_gitpath+"/git-rev-list").c_str(), "--reverse", branch.c_str(), "--");
		if (ret != 0) {
			throw PEX(str::printf("Unable to determine the root commit for branch '%s' (%d)", branch.c_str(), ret));
		}
		size_t pos = id.find_first_of('\n');
		if (pos == std::string::npos) {
			throw PEX(str::printf("Unable to determine the root commit for branch '%s' (%d)", branch.c_str(), ret));
		}

		root = id.substr(0, pos);
		PDEBUG << "Determined root commit in " << watch.elapsedMSecs() << " ms" << endl;
	}

	// Update the cache file
	std::string newfile = cachefile + ".tmp";
	FILE *out = fopen(newfile.c_str(), "w");
	if (out == NULL) {
		throw PEX_ERRNO();
	}
	fprintf(out, "%s %s %s\n", branch.c_str(), headrev.c_str(), root.c_str());
	{
		std::ifstream in(cachefile.c_str());
		while (in.good()) {
			std::string str;
			std::getline(in, str);
			if (str.empty() || !str.compare(0, branch.length(), branch)) {
				continue;
			}
			fprintf(out, "%s\n", str.c_str());
		}
		fsync(fileno(out));
		fclose(out);
	}
	sys::fs::rename(newfile, cachefile);

	return root;
}

// Returns the HEAD revision for the given branch
std::string GitBackend::head(const std::string &branch)
{
	int ret;
	std::string out = sys::io::exec(&ret, (m_gitpath+"/git-rev-list").c_str(), "-1", (branch.empty() ? "HEAD" : branch).c_str(), "--");
	if (ret != 0) {
		throw PEX(str::printf("Unable to retrieve head commit for branch %s (%d)", branch.c_str(), ret));
	}
	return str::trim(out);
}

// Returns the currently checked out branch
std::string GitBackend::mainBranch()
{
	int ret;
	std::string out = sys::io::exec(&ret, (m_gitpath+"/git-branch").c_str());
	if (ret != 0) {
		throw PEX(str::printf("Unable to retrieve the list of branches (%d)", ret));
	}
	std::vector<std::string> branches = str::split(out, "\n");
	for (unsigned int i = 0; i < branches.size(); i++) {
		if (branches[i].empty()) {
			continue;
		}
		if (branches[i][0] == '*') {
			return branches[i].substr(2);
		}
		branches[i] = branches[i].substr(2);
	}

	if (std::search_n(branches.begin(), branches.end(), 1, "master") != branches.end()) {
		return "master";
	} else if (std::search_n(branches.begin(), branches.end(), 1, "remotes/origin/master") != branches.end()) {
		return "remotes/origin/master";
	}

	// Fallback
	return "master";
}

// Returns a list of available local branches
std::vector<std::string> GitBackend::branches()
{
	int ret;
	std::string out = sys::io::exec(&ret, (m_gitpath+"/git-branch").c_str());
	if (ret != 0) {
		throw PEX(str::printf("Unable to retrieve the list of branches (%d)", ret));
	}
	std::vector<std::string> branches = str::split(out, "\n");
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
	std::string out = sys::io::exec(&ret, (m_gitpath+"/git-tag").c_str());
	if (ret != 0) {
		throw PEX(str::printf("Unable to retrieve the list of tags (%d)", ret));
	}
	std::vector<std::string> names = str::split(out, "\n");
	std::vector<Tag> tags;

	// Determine corresponding commits
	for (unsigned int i = 0; i < names.size(); i++) {
		if (names[i].empty()) {
			continue;
		}

		std::string out = sys::io::exec(&ret, (m_gitpath+"/git-rev-list").c_str(), "-1", names[i].c_str());
		if (ret != 0) {
			throw PEX(str::printf("Unable to retrieve the list of tags (%d)", ret));
		}

		std::string id = str::trim(out);
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
	if (m_prefetcher && m_prefetcher->willFetchDiffstat(id)) {
		Diffstat stat;
		if (!m_prefetcher->getDiffstat(id, &stat)) {
			throw PEX(str::printf("Failed to retrieve diffstat for revision %s", id.c_str()));
		}
		return stat;
	}

	PDEBUG << "Fetching revision " << id << " manually" << endl;

	std::vector<std::string> revs = str::split(id, ":");
	if (revs.size() > 1) {
		return GitDiffstatPipe::diffstat(m_gitpath, revs[1], revs[0]);
	}
	return GitDiffstatPipe::diffstat(m_gitpath, revs[0]);
}

// Returns a file listing for the given revision (defaults to HEAD)
std::vector<std::string> GitBackend::tree(const std::string &id)
{
	int ret;
	std::string out = sys::io::exec(&ret, (m_gitpath+"/git-ls-tree").c_str(), "-r", "--full-name", "--name-only", (id.empty() ? "HEAD" : id.c_str()));
	if (ret != 0) {
		throw PEX(str::printf("Unable to retrieve tree listing for ID '%s' (%d)", id.c_str(), ret));
	}
	std::vector<std::string> contents = str::split(out, "\n");
	while (!contents.empty() && contents[contents.size()-1].empty()) {
		contents.pop_back();
	}
	return contents;
}

// Returns the file contents of the given path at the given revision (defaults to HEAD)
std::string GitBackend::cat(const std::string &path, const std::string &id)
{
	int ret;
	std::string out = sys::io::exec(&ret, (m_gitpath+"/git-show").c_str(), ((id.empty() ? std::string("HEAD") : id)+":"+path).c_str());
	if (ret != 0) {
		throw PEX(str::printf("Unable to get file contents of %s@%s (%d)", path.c_str(), id.c_str(), ret));
	}
	return out;
}

// Returns a revision iterator for the given branch
Backend::LogIterator *GitBackend::iterator(const std::string &branch, int64_t start, int64_t end)
{
	int ret;
	std::string out;
	if (start >= 0) {
		std::string maxage = str::printf("--max-age=%lld", start);
		if (end >= 0) {
			std::string minage = str::printf("--min-age=%lld", end);
			out = sys::io::exec(&ret, (m_gitpath+"/git-rev-list").c_str(), "--first-parent", "--reverse", maxage.c_str(), minage.c_str(), branch.c_str(), "--");
		} else {
			out = sys::io::exec(&ret, (m_gitpath+"/git-rev-list").c_str(), "--first-parent", "--reverse", maxage.c_str(), branch.c_str(), "--");
		}
	} else {
		if (end >= 0) {
			std::string minage = str::printf("--min-age=%lld", end);
			out = sys::io::exec(&ret, (m_gitpath+"/git-rev-list").c_str(), "--first-parent", "--reverse", minage.c_str(), branch.c_str(), "--");
		} else {
			out = sys::io::exec(&ret, (m_gitpath+"/git-rev-list").c_str(), "--first-parent", "--reverse", branch.c_str(), "--");
		}
	}
	if (ret != 0) {
		throw PEX(str::printf("Unable to retrieve log for branch '%s' (%d)", branch.c_str(), ret));
	}
	std::vector<std::string> revisions = str::split(out, "\n");
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
		m_prefetcher = new GitRevisionPrefetcher(m_gitpath);
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
		throw PEX(str::printf("Unable to retrieve meta-data for revision '%s' (%d, %s)", id.c_str(), ret, meta.c_str()));
	}
	std::vector<std::string> lines = str::split(meta, "\n");
	int64_t date = 0;
	std::string author;
	if (!lines.empty()) {
		str::str2int(lines[0], &date);
		lines.erase(lines.begin());
	}
	if (!lines.empty()) {
		author = lines[0];
		lines.erase(lines.begin());
	}
	std::string msg = str::join(lines, "\n");
	return new Revision(id, date, author, msg, diffstat(id));
#else

	// Check for pre-fetched meta data first
	if (m_prefetcher && m_prefetcher->willFetchMeta(id)) {
		GitMetaDataThread::Data data;
		if (!m_prefetcher->getMeta(id, &data)) {
			throw PEX(str::printf("Failed to retrieve meta-data for revision %s", id.c_str()));
		}
		return new Revision(id, data.date, data.author, data.message, diffstat(id));
	}

	GitMetaDataThread::Data data;
	GitMetaDataThread::metaData(m_gitpath, utils::childId(id), &data);
	return new Revision(id, data.date, data.author, data.message, diffstat(id));
#endif
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
