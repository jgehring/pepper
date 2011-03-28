/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: subversion.cpp
 * Subversion repository backend
 */


#define __STDC_CONSTANT_MACROS // For INT64_C

#include "main.h"

#include <algorithm>
#include <cstdlib>
#include <stack>

#include <svn_client.h>
#include <svn_cmdline.h>
#include <svn_fs.h>
#include <svn_path.h>
#include <svn_pools.h>
#include <svn_ra.h>
#include <svn_sorts.h>
#include <svn_time.h>
#include <svn_utf.h>

#include "jobqueue.h"
#include "logger.h"
#include "options.h"
#include "revision.h"
#include "utils.h"

#include "syslib/fs.h"
#include "syslib/parallel.h"

#include "backends/subversion.h"
#include "backends/subversion_p.h"


// Constructor
SvnConnection::SvnConnection()
	: pool(NULL), ctx(NULL), ra(NULL), url(NULL), root(NULL), prefix(NULL)
{
}

// Destructor
SvnConnection::~SvnConnection()
{
	if (pool) {
		svn_pool_destroy(pool);
	}
}

// Opens the connection to the Subversion repository
void SvnConnection::open(const std::string &url, const std::map<std::string, std::string> &options)
{
	PTRACE << "Opening connection to " << url << endl;

	pool = svn_pool_create(NULL);
	init();

	// Canoicalize the repository URL
	this->url = svn_path_uri_encode(svn_path_canonicalize(url.c_str(), pool), pool);

	svn_error_t *err;
	// Create the client context
	if ((err = svn_client_create_context(&ctx, pool))) {
		throw PEX(strerr(err));
	}
	if ((err = svn_config_get_config(&(ctx->config), NULL, pool))) {
		throw PEX(strerr(err));
	}

	// Setup the authentication data
	svn_auth_baton_t *auth_baton;
	svn_config_t *config = hashget<svn_config_t *>(ctx->config, SVN_CONFIG_CATEGORY_CONFIG);
	const char *user = NULL, *pass = NULL;
	if (options.find("username") != options.end()) {
		user = apr_pstrdup(pool, options.find("username")->second.c_str());
	}
	if (options.find("password") != options.end()) {
		pass = apr_pstrdup(pool, options.find("password")->second.c_str());
	}
	if ((err = svn_cmdline_setup_auth_baton(&auth_baton, (options.find("non-interactive") != options.end()), user, pass, NULL, (options.find("no-auth-cache") != options.end()), config, NULL, NULL, pool))) {
		throw PEX(strerr(err));
	}
	ctx->auth_baton = auth_baton;

	// Setup the RA session
	if ((err = svn_client_open_ra_session(&ra, this->url, ctx, pool))) {
		throw PEX(strerr(err));
	}

	// Determine the repository root
	if ((err = svn_ra_get_repos_root2(ra, &root, pool))) {
		throw PEX(strerr(err));
	}
	prefix = apr_pstrdup(pool, this->url + strlen(root));

	PTRACE << "Root is " << root << " -> prefix is " << prefix << endl;
}

// Opens the connection to the Subversion repository, using the client context
// of the given parent connection
void SvnConnection::open(SvnConnection *parent)
{
	if (parent->ctx == NULL) {
		throw PEX("Parent connection not open yet.");
	}

	PTRACE << "Opening child connection to " << parent->url << endl;

	pool = svn_pool_create(NULL);
	init();

	// Copy connection data from parent
	ctx = parent->ctx;
	url = apr_pstrdup(pool, parent->url);
	root = apr_pstrdup(pool, parent->root);
	prefix = apr_pstrdup(pool, parent->prefix);

	// Setup the RA session
	svn_error_t *err;
	if ((err = svn_client_open_ra_session(&ra, url, ctx, pool))) {
		throw PEX(strerr(err));
	}
}

// Similar to svn_handle_error2(), but returns the error description as a std::string
std::string SvnConnection::strerr(svn_error_t *err)
{
	apr_pool_t *subpool;
	svn_error_t *tmp_err;
	apr_array_header_t *empties;

	apr_pool_create(&subpool, NULL);
	empties = apr_array_make(subpool, 0, sizeof(apr_status_t));

	std::string str;
	tmp_err = err;
	while (tmp_err) {
		int i;
		bool printed_already = false;

		if (!tmp_err->message) {
			for (i = 0; i < empties->nelts; i++) {
				if (tmp_err->apr_err == APR_ARRAY_IDX(empties, i, apr_status_t)) {
					printed_already = true;
					break;
				}
			}
		}

		if (!printed_already) {
			char errbuf[256];
			const char *err_string;
			svn_error_t *tmp_err2 = NULL;

			if (tmp_err->message) {
				if (!str.empty()) {
					str += ", ";
				}
				str += std::string(tmp_err->message);
			} else {
				if ((tmp_err->apr_err > APR_OS_START_USEERR) && (tmp_err->apr_err <= APR_OS_START_CANONERR)) {
					err_string = svn_strerror(tmp_err->apr_err, errbuf, sizeof(errbuf));
				} else if ((tmp_err2 = svn_utf_cstring_from_utf8(&err_string, apr_strerror(tmp_err->apr_err, errbuf, sizeof(errbuf)), tmp_err->pool))) {
					svn_error_clear(tmp_err2);
					err_string = "Can't recode error string from APR";
				}
				if (!str.empty()) {
					str += ", ";
				}
				str += std::string(err_string);
				APR_ARRAY_PUSH(empties, apr_status_t) = tmp_err->apr_err;
			}
		}

		tmp_err = tmp_err->child;
	}

	apr_pool_destroy(subpool);
	return str;
}

void SvnConnection::init()
{
	svn_error_t *err;
	if ((err = svn_fs_initialize(pool))) {
		throw PEX(strerr(err));
	}
	if ((err = svn_ra_initialize(pool))) {
		throw PEX(strerr(err));
	}
	if ((err = svn_config_ensure(NULL, pool))) {
		throw PEX(strerr(err));
	}
}


// Handles the prefetching of diffstats
class SvnDiffstatPrefetcher
{
public:
	SvnDiffstatPrefetcher(SvnConnection *connection, int n = 4)
	{
		Logger::info() << "SubversionBackend: Using " << n << " threads for prefetching diffstats" << endl;
		for (int i = 0; i < n; i++) {
			SvnDiffstatThread *thread = new SvnDiffstatThread(connection, &m_queue);
			thread->start();
			m_threads.push_back(thread);
		}
	}

	~SvnDiffstatPrefetcher()
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
	std::vector<SvnDiffstatThread *> m_threads;
};


// Constructor
SubversionBackend::SvnLogIterator::SvnLogIterator(SvnConnection *connection, const std::string &prefix, int64_t startrev, int64_t endrev)
	: Backend::LogIterator(), d(new SvnConnection()), m_prefix(prefix), m_startrev(startrev), m_endrev(endrev),
	  m_index(0), m_finished(false)
{
	d->open(connection);
}

// Desctructor
SubversionBackend::SvnLogIterator::~SvnLogIterator()
{
	delete d;
}

// Returns the next revision IDs, or an empty vector
bool SubversionBackend::SvnLogIterator::nextIds(std::queue<std::string> *queue)
{
	m_mutex.lock();
	while (m_index >= m_ids.size() && !m_finished) {
		m_cond.wait(&m_mutex);
	}

	if (m_index == m_ids.size()) {
		return false;
	}

	while (m_index < m_ids.size()) {
		queue->push(m_ids[m_index++]);
	}
	m_mutex.unlock();
	return true;
}

struct logReceiverBaton
{
	sys::parallel::Mutex *mutex;
	sys::parallel::WaitCondition *cond;
	std::vector<std::string> temp;
	std::vector<std::string> *ids;
	int64_t latest;
};

// Subversion callback for log messages
static svn_error_t *logReceiver(void *baton, svn_log_entry_t *entry, apr_pool_t *) 
{
	logReceiverBaton *b = static_cast<logReceiverBaton *>(baton);
	b->latest = entry->revision;
	b->temp.push_back(utils::int2str(b->latest));

	if (b->temp.size() > 64) {
		b->mutex->lock();
		for (unsigned int i = 0; i < b->temp.size(); i++) {
			b->ids->push_back(b->temp[i]);
		}
		b->temp.clear();
		b->cond->wakeAll();
		b->mutex->unlock();
	}
	return SVN_NO_ERROR;
}

// Main thread function
void SubversionBackend::SvnLogIterator::run()
{
	apr_pool_t *pool = svn_pool_create(d->pool);
	apr_pool_t *subpool = svn_pool_create(pool);
	apr_array_header_t *path = apr_array_make(pool, 1, sizeof (const char *));
	APR_ARRAY_PUSH(path, const char *) = svn_path_canonicalize(m_prefix.empty() ? "." : m_prefix.c_str(), pool);
	apr_array_header_t *props = apr_array_make(pool, 1, sizeof (const char *)); // Intentionally empty
	int windowSize = 1024;
	if (!strncmp(d->url, "file://", strlen("file://"))) {
		windowSize = 0;
	}

	logReceiverBaton baton;
	baton.mutex = &m_mutex;
	baton.cond = &m_cond;
	baton.ids = &m_ids;
	baton.latest = -1;

	// Determine revisions, but not at once
	int64_t wstart = m_startrev, lastStart = m_startrev;
	while (wstart < m_endrev-1) {
		PDEBUG << "Fetching log from " << wstart << " to " << m_endrev << " with window size " << windowSize << endl;
		svn_error_t *err = svn_ra_get_log2(d->ra, path, wstart, m_endrev, windowSize, FALSE, FALSE /* otherwise, copy history will be ignored */, FALSE, props, &logReceiver, &baton, subpool);
		if (err != NULL) {
			throw PEX(SvnConnection::strerr(err));
		}

		if (baton.latest + 1 > lastStart) {
			lastStart = baton.latest + 1;
		} else {
			lastStart += windowSize;
		}
		wstart = lastStart;
		svn_pool_clear(subpool);
	}

	m_mutex.lock();
	for (unsigned int i = 0; i < baton.temp.size(); i++) {
		m_ids.push_back(baton.temp[i]);
	}
	m_finished = true;
	m_cond.wakeAll();
	m_mutex.unlock();

	svn_pool_destroy(pool);
}


// Constructor
SubversionBackend::SubversionBackend(const Options &options)
	: Backend(options), d(new SvnConnection()), m_prefetcher(NULL)
{

}

// Destructor
SubversionBackend::~SubversionBackend()
{
	if (m_prefetcher) {
		m_prefetcher->stop();
		m_prefetcher->wait();
		delete m_prefetcher;
	}
	delete d;
}

// Opens the connection to the repository
void SubversionBackend::init()
{
	// Initialize the Subversion C library
#ifndef WIN32
	if (svn_cmdline_init(PACKAGE, stderr) != EXIT_SUCCESS) {
		throw PEX("Failed to initialize Subversion library");
	}
	atexit(apr_terminate);
#else // !WIN32
	if (svn_cmdline_init(PACKAGE, NULL) != EXIT_SUCCESS) {
		throw PEX("Failed to initialize Subversion library");
	}
#endif // !WIN32

	PDEBUG << "Subversion library initialized, opening connection" << endl;

	std::string url = m_opts.repository();
	if (!url.compare(0, 1, "/")) {
		url = std::string("file://") + url;
	}
	d->open(url, m_opts.options());
}

// Returns true if this backend is able to access the given repository
bool SubversionBackend::handles(const std::string &url)
{
	const char *schemes[] = {"svn://", "svn+ssh://", "http://", "https://", "file://"};
	for (unsigned int i = 0; i < sizeof(schemes)/sizeof(schemes[0]); i++) {
		if (url.compare(0, strlen(schemes[i]), schemes[i]) == 0) {
			return true;
		}
	}
	// Local repository without the 'file://' prefix
	if (sys::fs::dirExists(url) && sys::fs::dirExists(url+"/locks") && sys::fs::exists(url+"/db/uuid")) {
		return true;
	}
	return false;
}

// Returns a unique identifier for this repository
std::string SubversionBackend::uuid()
{
	apr_pool_t *pool = svn_pool_create(d->pool);
	const char *id;
	svn_error_t *err = svn_ra_get_uuid2(d->ra, &id, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}
	std::string sid(id);
	svn_pool_destroy(pool);
	return sid;
}

// Returns the HEAD revision for the current branch
std::string SubversionBackend::head(const std::string &branch)
{
	std::string prefix;
	if (branch == "trunk") {
		prefix = m_opts.value("trunk", "trunk");
	} else if (!branch.empty()) {
		prefix = m_opts.value("branches", "branches");
		prefix += "/";
		prefix += branch;
	}
	PDEBUG << "branch = " << branch << " -> prefix = " << prefix << endl;

	apr_pool_t *pool = svn_pool_create(d->pool);
	svn_dirent_t *dirent;
	svn_error_t *err = svn_ra_stat(d->ra, prefix.c_str(), SVN_INVALID_REVNUM, &dirent, pool);
	if (err == NULL && dirent == NULL && branch == "trunk") {
		err = svn_ra_stat(d->ra, "", SVN_INVALID_REVNUM, &dirent, pool);
	}
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}
	if (dirent == NULL) {
		throw PEX(std::string("Unable to determine HEAD revision of ")+d->url+"/"+prefix);
	}

	svn_revnum_t rev = dirent->created_rev;
	PDEBUG << "Head revision for branch " << prefix << " is " << rev << endl;
	svn_pool_destroy(pool);
	return utils::int2str((long int)rev);
}

// Returns the standard branch (i.e., trunk)
std::string SubversionBackend::mainBranch()
{
	return "trunk";
}

// Returns a list of available branches
std::vector<std::string> SubversionBackend::branches()
{
	std::string prefix = m_opts.value("branches", "branches");
	std::vector<std::string> branches;

	apr_pool_t *pool = svn_pool_create(d->pool);

	// Check if branches directoy is present
	svn_dirent_t *dirent;
	svn_error_t *err = svn_ra_stat(d->ra, prefix.c_str(), SVN_INVALID_REVNUM, &dirent, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	if (dirent == NULL || dirent->kind != svn_node_dir) {
		// It's not here; let's call the main branch "trunk"
		branches.push_back("trunk");
		return branches;
	}

	// Get directory entries
	apr_hash_t *dirents;
	err = svn_ra_get_dir2(d->ra, &dirents, NULL, NULL, prefix.c_str(), dirent->created_rev, SVN_DIRENT_KIND, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	branches.push_back("trunk");
	for (apr_hash_index_t *hi = apr_hash_first(pool, dirents); hi; hi = apr_hash_next(hi)) {
		const char *entry;
		svn_dirent_t *dirent;
		apr_hash_this(hi, (const void **)(void *)&entry, NULL, (void **)(void *)&dirent);
		if (dirent->kind == svn_node_dir) {
			branches.push_back(std::string(entry));
		}
	}

	// Let's be nice
	std::sort(branches.begin()+1, branches.end());
	svn_pool_clear(pool);
	return branches;
}

// Returns a list of available tags
std::vector<Tag> SubversionBackend::tags()
{
	std::string prefix = m_opts.value("tags", "tags");
	std::vector<Tag> tags;

	apr_pool_t *pool = svn_pool_create(d->pool);

	// Check if tags directoy is present
	svn_dirent_t *dirent;
	svn_error_t *err = svn_ra_stat(d->ra, prefix.c_str(), SVN_INVALID_REVNUM, &dirent, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	if (dirent == NULL || dirent->kind != svn_node_dir) {
		// No tags here
		return tags;
	}

	// Get directory entries
	apr_hash_t *dirents;
	err = svn_ra_get_dir2(d->ra, &dirents, NULL, NULL, prefix.c_str(), dirent->created_rev, SVN_DIRENT_KIND | SVN_DIRENT_CREATED_REV, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	apr_array_header_t *array = svn_sort__hash(dirents, &svn_sort_compare_items_lexically,  pool);
	for (int i = 0; i < array->nelts; i++) {
		svn_sort__item_t *item = &APR_ARRAY_IDX(array, i, svn_sort__item_t);
		svn_dirent_t *dirent = (svn_dirent_t *)apr_hash_get(dirents, item->key, item->klen);
		if (dirent->kind == svn_node_dir) {
			tags.push_back(Tag(utils::int2str(dirent->created_rev), (const char *)item->key));
		}
	}

	svn_pool_clear(pool);
	return tags;
}

// Returns a diffstat for the specified revision
Diffstat SubversionBackend::diffstat(const std::string &id)
{
	if (m_prefetcher && m_prefetcher->willFetch(id)) {
		PDEBUG << "Revision " << id << " will be prefetched" << endl;
		Diffstat stat;
		if (!m_prefetcher->get(id, &stat)) {
			throw PEX(std::string("Failed to retrieve diffstat for revision ") + id);
		}
		return stat;
	}


	std::vector<std::string> revs = utils::split(id, ":");
	svn_revnum_t r1, r2;
	if (revs.size() > 1) {
		if (!utils::str2int(revs[0], &r1) || !utils::str2int(revs[1], &r2)) {
			throw PEX(std::string("Error parsing revision number ") + id);
		}
	} else {
		if (!utils::str2int(revs[0], &r2)) {
			throw PEX(std::string("Error parsing revision number ") + id);
		}
		r1 = r2 - 1;
	}

	PDEBUG << "Fetching revision " << id << " manually" << endl;

	apr_pool_t *subpool = svn_pool_create(d->pool);
	Diffstat stat = SvnDiffstatThread::diffstat(d, r1, r2, subpool);
	svn_pool_destroy(subpool);
	return stat;
}

// Filters a diffstat by prefix
void SubversionBackend::filterDiffstat(Diffstat *stat)
{
	// Strip prefix
	if (strlen(d->prefix) > 1) {
		stat->filter(d->prefix + 1);
	}
}

// Returns a file listing for the given revision (defaults to HEAD)
std::vector<std::string> SubversionBackend::tree(const std::string &id)
{
	svn_revnum_t revision;
	if (id.empty()) {
		revision = SVN_INVALID_REVNUM;
	} else if (!utils::str2int(id, &revision)) {
		throw PEX(std::string("Error parsing revision number ") + id);
	}

	apr_pool_t *pool = svn_pool_create(d->pool);

	std::vector<std::string> contents;

	// Pseudo-recursively list directory entries
	std::stack<std::pair<std::string, int> > stack;
	stack.push(std::pair<std::string, int>("", svn_node_dir));
	while (!stack.empty()) {
		apr_pool_t *subpool = svn_pool_create(pool);

		std::string node = stack.top().first;
		if (stack.top().second != svn_node_dir) {
			contents.push_back(node);
			stack.pop();
			continue;
		}
		stack.pop();

		PTRACE << "Listing directory contents in " << node << "@" << revision << endl;

		apr_hash_t *dirents;
		svn_error_t *err = svn_ra_get_dir2(d->ra, &dirents, NULL, NULL, node.c_str(), revision, SVN_DIRENT_KIND, subpool);
		if (err != NULL) {
			throw PEX(SvnConnection::strerr(err));
		}

		std::string prefix = (node.empty() ? "" : node + "/");
		std::stack<std::pair<std::string, int> > next;
		apr_array_header_t *array = svn_sort__hash(dirents, &svn_sort_compare_items_lexically,  subpool);
		for (int i = 0; i < array->nelts; i++) {
			svn_sort__item_t *item = &APR_ARRAY_IDX(array, i, svn_sort__item_t);
			svn_dirent_t *dirent = (svn_dirent_t *)apr_hash_get(dirents, item->key, item->klen);
			if (dirent->kind == svn_node_file || dirent->kind == svn_node_dir) {
				next.push(std::pair<std::string, int>(prefix + (const char *)item->key, dirent->kind));
			}
		}
		while (!next.empty()) {
			stack.push(next.top());
			next.pop();
		}

		svn_pool_destroy(subpool);
	}

	svn_pool_destroy(pool);
	return contents;
}

// Returns a log iterator for the given branch
Backend::LogIterator *SubversionBackend::iterator(const std::string &branch, int64_t start, int64_t end)
{
	std::string prefix;
	if (branch == "trunk") {
		prefix = m_opts.value("trunk", "trunk");
	} else if (!branch.empty()) {
		prefix = m_opts.value("branches", "branches");
		prefix += "/";
		prefix += branch;
	}

	PDEBUG << "Iterator requested for branch " << branch << " -> prefix = " << prefix << endl;

	// Check if the branch exists
	apr_pool_t *pool = svn_pool_create(d->pool);
	svn_dirent_t *dirent;
	svn_error_t *err = svn_ra_stat(d->ra, prefix.c_str(), SVN_INVALID_REVNUM, &dirent, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}
	if (dirent == NULL) {
		if (prefix == "trunk") {
			prefix.clear();
		} else {
			throw PEX(utils::strprintf("No such branch: %s", branch.c_str()));
		}
	}

	svn_revnum_t startrev = 0, endrev;
	apr_time_t time;
	if (start >= 0) {
		apr_time_ansi_put(&time, start);
		if ((err = svn_ra_get_dated_revision(d->ra, &startrev, time, pool)) != NULL) {
			throw PEX(SvnConnection::strerr(err));
		}
	}
	if (end >= 0) {
		apr_time_ansi_put(&time, end);
		if ((err = svn_ra_get_dated_revision(d->ra, &endrev, time, pool)) != NULL) {
			throw PEX(SvnConnection::strerr(err));
		}
	} else {
		utils::str2int(head(branch), &endrev);
	}

	PDEBUG << "Revision range: [ " << start << " : " << end << " -> [" << startrev << " : " << endrev << "]" << endl;

	svn_pool_destroy(pool);
	return new SvnLogIterator(d, prefix, startrev, endrev);
}

// Adds the given revision IDs to the diffstat scheduler
void SubversionBackend::prefetch(const std::vector<std::string> &ids)
{
	if (m_prefetcher == NULL) {
		std::string numthreads = m_opts.value("threads", "10");
		int nthreads = 10;
		if (!utils::str2int(numthreads, &nthreads)) {
			throw PEX(std::string("Expected number for --threads parameter: ") + numthreads);
		}

		// Don't use that many threads for local repositories
		if (!strncmp(d->url, "file://", strlen("file://"))) {
			nthreads = std::max(1, sys::parallel::idealThreadCount() / 2);
		}
		m_prefetcher = new SvnDiffstatPrefetcher(d, nthreads);
	}
	m_prefetcher->prefetch(ids);
}

// Handle cleanup of diffstat scheduler
void SubversionBackend::finalize()
{
	if (m_prefetcher) {
		m_prefetcher->stop();
		m_prefetcher->wait();
		delete m_prefetcher;
		m_prefetcher = NULL;
	}
}

// Prints a help screen
void SubversionBackend::printHelp() const
{
	Options::print("--username=ARG", "Specify a username ARG");
	Options::print("--password=ARG", "Specify a password ARG");
	Options::print("--no-auth-cache", "Do not cache authentication tokens");
	Options::print("--non-interactive", "Do no interactive prompting");
	Options::print("--trunk=ARG", "Trunk is at subdirectory ARG");
	Options::print("--branches=ARG", "Branches are in subdirectory ARG");
	Options::print("--tags=ARG", "Tags are in subdirectory ARG");
	Options::print("--threads=ARG", "Use ARG threads for requesting diffstats");
}

// Returns the revision data for the given ID
Revision *SubversionBackend::revision(const std::string &id)
{
	std::map<std::string, std::string> data;
	std::string rev = utils::split(id, ":").back();

	svn_revnum_t revnum;
	if (!utils::str2int(rev, &(revnum))) {
		throw PEX(std::string("Error parsing revision number ") + id);
	}

	apr_pool_t *pool = svn_pool_create(d->pool);
	apr_hash_t *props;

	svn_error_t *err = svn_ra_rev_proplist(d->ra, revnum, &props, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	svn_string_t *value;
	std::string author, message;
	int64_t date = 0;
	if ((value = static_cast<svn_string_t *>(apr_hash_get(props, "svn:author", APR_HASH_KEY_STRING)))) {
		author = value->data;
	}
	if ((value = static_cast<svn_string_t *>(apr_hash_get(props, "svn:date", APR_HASH_KEY_STRING)))) {
		apr_time_t when;
		if ((err = svn_time_from_cstring(&when, value->data, pool)) != NULL) {
			throw PEX(SvnConnection::strerr(err));
		}
		date = apr_time_sec(when);
	}
	if ((value = static_cast<svn_string_t *>(apr_hash_get(props, "svn:log", APR_HASH_KEY_STRING)))) {
		message = value->data;
	}

	svn_pool_destroy(pool);
	return new Revision(id, date, author, message, diffstat(id));
}
