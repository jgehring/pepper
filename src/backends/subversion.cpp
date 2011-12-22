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

#include "bstream.h"
#include "cache.h"
#include "jobqueue.h"
#include "logger.h"
#include "options.h"
#include "revision.h"
#include "strlib.h"

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

	// Determine the repository root and reparent
	if ((err = svn_ra_get_repos_root2(ra, &root, pool))) {
		throw PEX(strerr(err));
	}
	prefix = apr_pstrdup(pool, this->url + strlen(root));
	if (prefix && *prefix == '/') {
		++prefix; // Original adress is tracked in pool
	}
	PDEBUG << "Root is " << root << " -> prefix is " << prefix << endl;

	PTRACE << "Reparent to " << root << endl;
	if ((err = svn_ra_reparent(ra, root, pool))) {
		throw PEX(strerr(err));
	}
}

// Opens the connection to the Subversion repository, using the client context
// of the given parent connection
void SvnConnection::open(SvnConnection *parent)
{
	if (parent->ctx == NULL) {
		throw PEX("Parent connection not open yet.");
	}
	PTRACE << "Opening child connection to " << parent->root << endl;

	pool = svn_pool_create(NULL);
	init();

	// Copy connection data from parent
	ctx = parent->ctx;
	url = apr_pstrdup(pool, parent->url);
	root = apr_pstrdup(pool, parent->root);
	prefix = apr_pstrdup(pool, parent->prefix);

	// Setup the RA session
	svn_error_t *err;
	if ((err = svn_client_open_ra_session(&ra, root, ctx, pool))) {
		throw PEX(strerr(err));
	}
}

// Similar to svn_handle_error2(), but returns the error description as a std::string
std::string SvnConnection::strerr(svn_error_t *err)
{
	apr_pool_t *pool;
	svn_error_t *tmp_err;
	apr_array_header_t *empties;

	apr_pool_create(&pool, NULL);
	empties = apr_array_make(pool, 0, sizeof(apr_status_t));

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

	apr_pool_destroy(pool);
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


// Static mutex for reading and writing log interavals
sys::parallel::Mutex SubversionBackend::SvnLogIterator::s_cacheMutex;

// Constructor
SubversionBackend::SvnLogIterator::SvnLogIterator(SubversionBackend *backend, const std::string &prefix, uint64_t startrev, uint64_t endrev)
	: Backend::LogIterator(), m_backend(backend), d(new SvnConnection()), m_prefix(prefix), m_startrev(startrev), m_endrev(endrev),
	  m_index(0), m_finished(false), m_failed(false)
{
	d->open(m_backend->d);
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

	if (m_failed) {
		throw PEX("Error fetching server log");
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
	uint64_t latest;
};

// Subversion callback for log messages
static svn_error_t *logReceiver(void *baton, svn_log_entry_t *entry, apr_pool_t *)
{
	logReceiverBaton *b = static_cast<logReceiverBaton *>(baton);
	b->latest = entry->revision;
	b->temp.push_back(str::itos(b->latest));

	if (b->temp.size() > 64) {
		b->mutex->lock();
		for (size_t i = 0; i < b->temp.size(); i++) {
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
	apr_pool_t *iterpool = svn_pool_create(pool);
	apr_array_header_t *path = apr_array_make(pool, 1, sizeof (const char *));
	std::string sessionPrefix = d->prefix;
	if (m_prefix.empty()) {
		APR_ARRAY_PUSH(path, const char *) = svn_path_canonicalize(sessionPrefix.c_str(), pool);
	} else if (sessionPrefix.empty()) {
		APR_ARRAY_PUSH(path, const char *) = svn_path_canonicalize(m_prefix.c_str(), pool);
	} else {
		APR_ARRAY_PUSH(path, const char *) = svn_path_canonicalize((sessionPrefix+"/"+m_prefix).c_str(), pool);
	}
	apr_array_header_t *props = apr_array_make(pool, 1, sizeof (const char *)); // Intentionally empty

	int windowSize = 1024;
	if (!strncmp(d->url, "file://", strlen("file://"))) {
		windowSize = 0;
	}

	PDEBUG << "Path is " << APR_ARRAY_IDX(path, 0, const char *)
		<< ", range is [" << m_startrev << ":" << m_endrev << "]" << endl;

	// This is a vector of intervals that should be fetched. The revisions of
	// each interval will be appended after fetching, so it can be used to store
	// revisions that are already cached.
	std::vector<Interval> fetch;
	fetch.push_back(Interval(m_startrev, m_endrev));

	std::string cachefile = str::printf("log_%s", APR_ARRAY_IDX(path, 0, const char *));
	if (m_backend->options().useCache()) {
		readIntervalsFromCache(cachefile);

		// Check if a cached interval intersects with the interval that should be fetched. It's
		// assumed that m_cachedIntervals doesn't contain intersecting intervals, which is assured
		// by the recursive merging in mergeInterval().
		std::vector<Interval> fetchnew = missingIntervals(m_startrev, m_endrev);
		if (!fetchnew.empty()) {
			fetch = fetchnew;
		}
	}

	logReceiverBaton baton;
	baton.mutex = &m_mutex;
	baton.cond = &m_cond;
	baton.ids = &m_ids;
	baton.latest = 0;

	// Fetch all revision intervals that are required
	for (size_t i = 0; i < fetch.size(); i++) {
		svn_pool_clear(iterpool);

		uint64_t wstart = fetch[i].start, lastStart = fetch[i].start;
		while (wstart <= fetch[i].end) {
			PDEBUG << "Fetching log from " << wstart << " to " << fetch[i].end << " with window size " << windowSize << endl;
			svn_error_t *err = svn_ra_get_log2(d->ra, path, wstart, fetch[i].end, windowSize, FALSE, FALSE /* otherwise, copy history will be ignored */, FALSE, props, &logReceiver, &baton, iterpool);
			if (err != NULL) {
				Logger::err() << "Error: Unable to fetch server log: " << SvnConnection::strerr(err) << endl;
				m_mutex.lock();
				m_failed = true;
				m_finished = true;
				m_cond.wakeAll();
				m_mutex.unlock();
				svn_pool_destroy(pool);
				return;
			}

			if (baton.latest + 1 > lastStart) {
				lastStart = baton.latest + 1;
			} else {
				lastStart += std::max(windowSize, 1);
			}
			wstart = lastStart;
		}

		m_mutex.lock();
		Logger &l = PTRACE << "Appending " << baton.temp.size() << " fetched revisions: ";
		for (size_t j = 0; j < baton.temp.size(); j++) {
			l << baton.temp[j] << " ";
			m_ids.push_back(baton.temp[j]);
		}
		l << endl;
		baton.temp.clear();

		// Append cached revisions
		if (!fetch[i].revisions.empty()) {
			l << "Appending " << fetch[i].revisions.size() << " cached revisions: ";
			for (size_t j = 0; j < fetch[i].revisions.size(); j++) {
				l << fetch[i].revisions[j] << " ";
				m_ids.push_back(str::itos(fetch[i].revisions[j]));
			}
			l << endl;
		}

		m_cond.wakeAll();
		m_mutex.unlock();
	}

	m_mutex.lock();
	m_finished = true;
	m_cond.wakeAll();
	m_mutex.unlock();

	if (m_backend->options().useCache()) {
		// Add current interval
		Interval current(m_startrev, m_endrev);
		for (size_t i = 0; i < m_ids.size(); i++) {
			uint64_t rev;
			str::stoi(m_ids[i], &rev);
			current.revisions.push_back(rev);
		}

		mergeInterval(current);
		writeIntervalsToCache(cachefile);
	}

	svn_pool_destroy(pool);
}

// Reads previous log intervals from the cache
void SubversionBackend::SvnLogIterator::readIntervalsFromCache(const std::string &file)
{
	sys::parallel::MutexLocker locker(&s_cacheMutex);
	std::string cachefile = Cache::cacheFile(m_backend, file);

	m_cachedIntervals.clear();

	if (!sys::fs::fileExists(cachefile)) {
		return;
	}

	GZIStream in(cachefile);
	uint32_t version;
	in >> version;
	if (version != 1) {
		Logger::warn() << "Unknown version number in cache file " << cachefile  << ": " << version << endl;
		return;
	}
	while (!in.eof()) {
		Interval interval;
		uint64_t i = 0, num;
		in >> interval.start >> interval.end >> num;
		interval.revisions.resize(num);
		while (i < num && !in.eof()) {
			in >> interval.revisions[i++];
		}

		if (interval.revisions.empty() || interval.start >= interval.end) {
			PTRACE << "Skipping bogus interval: [" << interval.start << ":" << interval.end
				<< "] with " << interval.revisions.size() << " revisions" << endl;
			continue;
		}

		PTRACE << "New revision range: [" << interval.start << ":" << interval.end
			<< "] with " << interval.revisions.size() << " revisions" << endl;
		m_cachedIntervals.push_back(interval);
	}

	if (!in.ok()) {
		m_cachedIntervals.clear();
		Logger::warn() << "Error reading from cache file " << cachefile << endl;
	}
}

// Writes the current intervals to the cache
void SubversionBackend::SvnLogIterator::writeIntervalsToCache(const std::string &file)
{
	sys::parallel::MutexLocker locker(&s_cacheMutex);
	std::string cachefile = Cache::cacheFile(m_backend, file);

	PDEBUG << "Writing log intervals to cache file " << cachefile << endl;
	GZOStream *out = new GZOStream(cachefile);
	*out << (uint32_t)1; // Version number
	for (size_t i = 0; i < m_cachedIntervals.size() && out->ok(); i++) {
		const std::vector<uint64_t> &revisions = m_cachedIntervals[i].revisions;
		*out << m_cachedIntervals[i].start << m_cachedIntervals[i].end << (uint64_t)revisions.size();
		for (size_t j = 0; j < revisions.size() && out->ok(); j++) {
			*out << revisions[j];
		}
	}
	if (!out->ok()) {
		Logger::warn() << "Error writing to cache file: " << cachefile << endl;
		delete out;
		out = NULL;
		sys::fs::unlink(cachefile);
	}
	delete out;
}

// Merges the given interval into the intervals that have been already known
void SubversionBackend::SvnLogIterator::mergeInterval(const Interval &interval)
{
	PDEBUG << "Merging new interval [" << interval.start << ":" << interval.end << "]" << endl;
	for (size_t i = 0; i < m_cachedIntervals.size(); i++) {
		const Interval &candidate = m_cachedIntervals[i];
		if ((interval.start <= candidate.start && interval.end >= candidate.start) ||
			(interval.start <= candidate.end && interval.end >= candidate.end)) {
			// Intervals intersect, let's merge them.
			// NOTE: It's assumed that the repository is immutable, i.e. no
			// revisions will ever be deleted.
			PDEBUG << "Intervals [" << interval.start << ":" << interval.end << "] and ["
				<< candidate.start << ":" << candidate.end << "] intersect" << endl;

			Interval merged(std::min(interval.start, candidate.start), std::max(interval.end, candidate.end));
			merged.revisions.insert(merged.revisions.begin(), interval.revisions.begin(), interval.revisions.end());
			merged.revisions.insert(merged.revisions.end(), candidate.revisions.begin(), candidate.revisions.end());
			std::sort(merged.revisions.begin(), merged.revisions.end());

			// Remove duplicates
			for (size_t j = 0; j < merged.revisions.size()-1; j++) {
				if (merged.revisions[j] == merged.revisions[j+1]) {
					merged.revisions.erase(merged.revisions.begin()+j);
					--j;
				}
			}

			// Merge the merged interval
			m_cachedIntervals.erase(m_cachedIntervals.begin()+i);
			mergeInterval(merged);
			return;
		}
	}

	PDEBUG << "No intersections" << endl;
	// No merge with previous intervals
	m_cachedIntervals.push_back(interval);
}

// Returns missing intervals, including following cached revision numbers
std::vector<SubversionBackend::SvnLogIterator::Interval> SubversionBackend::SvnLogIterator::missingIntervals(uint64_t start, uint64_t end)
{
	std::vector<Interval> missing;

	for (size_t i = 0; i < m_cachedIntervals.size(); i++) {
		const Interval &candidate = m_cachedIntervals[i];
		if (start >= candidate.start && end <= candidate.end ) {
			// Completely inside candidate interval. No need to fetch any logs from the repository,
			// but store the relevant revisions in a pseudo interval
			Interval interval(start+1, start);
			size_t j = 0, n = candidate.revisions.size();
			while (j < n && candidate.revisions[j] < start) ++j;
			while (j < n && candidate.revisions[j] <= end) {
				interval.revisions.push_back(candidate.revisions[j]);
				++j;
			}
			missing.push_back(interval);
			return missing;
		}
		else if (start <= candidate.start && end >= candidate.end) {
			// Completely including candidate interval. Two intervals need to be fetched from
			// the server now, but both will be splitted further.
			std::vector<Interval> recurse = missingIntervals(start, (candidate.start == 0 ? 0 : candidate.start-1));
			recurse.back().revisions.insert(recurse.back().revisions.end(), candidate.revisions.begin(), candidate.revisions.end());
			missing.insert(missing.begin(), recurse.begin(), recurse.end());

			recurse = missingIntervals(candidate.end+1, end);
			missing.insert(missing.end(), recurse.begin(), recurse.end());
			return missing;
		}
		else if (start >= candidate.start && start <= candidate.end) {
			// Intersecting at start. Add a pseudo interval for the cached revisions
			// and an interval for the remaining ones after candidate.end
			Interval interval1(start+1, start);
			size_t j = 0, n = candidate.revisions.size();
			while (j < n && candidate.revisions[j] < start) ++j;
			interval1.revisions.insert(interval1.revisions.begin(), candidate.revisions.begin() + j, candidate.revisions.end());
			missing.push_back(interval1);

			std::vector<Interval> recurse = missingIntervals(candidate.end+1, end);
			missing.insert(missing.end(), recurse.begin(), recurse.end());
			return missing;
		}
		else if (end >= candidate.start && end <= candidate.end) {
			// Intersectiong at end. Add a single interval, including the first
			// revisions in candidate
			std::vector<Interval> recurse = missingIntervals(start, (candidate.start == 0 ? 0 : candidate.start-1));
			for (size_t j = 0; j < candidate.revisions.size() && candidate.revisions[j] <= end; j++) {
				recurse.back().revisions.push_back(candidate.revisions[j]);
			}
			missing.insert(missing.begin(), recurse.begin(), recurse.end());
			return missing;
		}
	}

	// No intersections, so return the complete interval
	missing.push_back(Interval(start, end));
	return missing;
}


// Constructor
SubversionBackend::SubversionBackend(const Options &options)
	: Backend(options), d(new SvnConnection()), m_prefetcher(NULL)
{

}

// Destructor
SubversionBackend::~SubversionBackend()
{
	close();
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
		// Detect working copy
		apr_pool_t *pool = svn_pool_create(NULL);
		const char *repo;
		svn_error_t *err = svn_client_url_from_path(&repo, url.c_str(), pool);
		if (err == NULL) {
			url = std::string(repo);
		} else {
			// Local repository
			url = std::string("file://") + url;
		}
		svn_pool_destroy(pool);
	}
	d->open(url, m_opts.options());
}

// Called after Report::run()
void SubversionBackend::close()
{
	// Clean up any prefetching threads
	finalize();
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

	// Working copy with .svn directory
	if (sys::fs::dirExists(url+"/.svn")) {
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
	apr_pool_t *pool = svn_pool_create(d->pool);
	std::string prefix = this->prefix(branch, pool);
	svn_dirent_t *dirent;
	svn_error_t *err = svn_ra_stat(d->ra, svn_path_join(d->prefix, prefix.c_str(), pool), SVN_INVALID_REVNUM, &dirent, pool);
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
	return str::itos((long int)rev);
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
	char *absPrefix = svn_path_join(d->prefix, prefix.c_str(), pool);

	// Check if branches directoy is present
	svn_dirent_t *dirent;
	svn_error_t *err = svn_ra_stat(d->ra, absPrefix, SVN_INVALID_REVNUM, &dirent, pool);
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
	err = svn_ra_get_dir2(d->ra, &dirents, NULL, NULL, absPrefix, dirent->created_rev, SVN_DIRENT_KIND, pool);
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
	svn_pool_destroy(pool);
	return branches;
}

// Returns a list of available tags
std::vector<Tag> SubversionBackend::tags()
{
	std::string prefix = m_opts.value("tags", "tags");
	std::vector<Tag> tags;

	apr_pool_t *pool = svn_pool_create(d->pool);
	char *absPrefix = svn_path_join(d->prefix, prefix.c_str(), pool);

	// Check if tags directoy is present
	svn_dirent_t *dirent;
	svn_error_t *err = svn_ra_stat(d->ra, absPrefix, SVN_INVALID_REVNUM, &dirent, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	if (dirent == NULL || dirent->kind != svn_node_dir) {
		// No tags here
		return tags;
	}

	// Get directory entries
	apr_hash_t *dirents;
	err = svn_ra_get_dir2(d->ra, &dirents, NULL, NULL, absPrefix, dirent->created_rev, SVN_DIRENT_KIND | SVN_DIRENT_CREATED_REV, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	apr_array_header_t *array = svn_sort__hash(dirents, &svn_sort_compare_items_lexically,  pool);
	for (int i = 0; i < array->nelts; i++) {
		svn_sort__item_t *item = &APR_ARRAY_IDX(array, i, svn_sort__item_t);
		svn_dirent_t *dirent = (svn_dirent_t *)apr_hash_get(dirents, item->key, item->klen);
		if (dirent->kind == svn_node_dir) {
			tags.push_back(Tag(str::itos(dirent->created_rev), (const char *)item->key));
		}
	}

	svn_pool_destroy(pool);
	return tags;
}

// Returns a diffstat for the specified revision
Diffstat SubversionBackend::diffstat(const std::string &id)
{
	if (m_prefetcher && m_prefetcher->willFetch(id)) {
		PTRACE << "Revision " << id << " will be prefetched" << endl;
		Diffstat stat;
		if (!m_prefetcher->get(id, &stat)) {
			throw PEX(std::string("Failed to retrieve diffstat for revision ") + id);
		}
		return stat;
	}


	std::vector<std::string> revs = str::split(id, ":");
	svn_revnum_t r1, r2;
	if (revs.size() > 1) {
		if (!str::stoi(revs[0], &r1) || !str::stoi(revs[1], &r2)) {
			throw PEX(std::string("Error parsing revision number ") + id);
		}
	} else {
		if (!str::stoi(revs[0], &r2)) {
			throw PEX(std::string("Error parsing revision number ") + id);
		}
		r1 = r2 - 1;
	}

	PDEBUG << "Fetching revision " << id << " manually" << endl;

	apr_pool_t *pool = svn_pool_create(d->pool);
	Diffstat stat = SvnDiffstatThread::diffstat(d, r1, r2, pool);
	svn_pool_destroy(pool);
	return stat;
}

// Filters a diffstat by prefix
void SubversionBackend::filterDiffstat(Diffstat *stat)
{
	// Strip prefix
	if (d->prefix && strlen(d->prefix)) {
		stat->filter(d->prefix);
	}
}

// Returns a file listing for the given revision (defaults to HEAD)
std::vector<std::string> SubversionBackend::tree(const std::string &id)
{
	svn_revnum_t revision;
	if (id.empty()) {
		revision = SVN_INVALID_REVNUM;
	} else if (!str::stoi(id, &revision)) {
		throw PEX(std::string("Error parsing revision number ") + id);
	}

	apr_pool_t *pool = svn_pool_create(d->pool);
	apr_pool_t *iterpool = svn_pool_create(pool);

	std::vector<std::string> contents;

	// Pseudo-recursively list directory entries
	std::stack<std::pair<std::string, int> > stack;
	stack.push(std::pair<std::string, int>("", svn_node_dir));
	while (!stack.empty()) {
		svn_pool_clear(iterpool);

		std::string node = stack.top().first;
		if (stack.top().second != svn_node_dir) {
			contents.push_back(node);
			stack.pop();
			continue;
		}
		stack.pop();

		PTRACE << "Listing directory contents in " << node << "@" << revision << endl;

		apr_hash_t *dirents;
		svn_error_t *err = svn_ra_get_dir2(d->ra, &dirents, NULL, NULL, node.c_str(), revision, SVN_DIRENT_KIND, iterpool);
		if (err != NULL) {
			throw PEX(SvnConnection::strerr(err));
		}

		std::string prefix = (node.empty() ? "" : node + "/");
		std::stack<std::pair<std::string, int> > next;
		apr_array_header_t *array = svn_sort__hash(dirents, &svn_sort_compare_items_lexically,  iterpool);
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
	}

	svn_pool_destroy(pool);
	return contents;
}

// Returns the file contents of the given path at the given revision (defaults to HEAD)
std::string SubversionBackend::cat(const std::string &path, const std::string &id)
{
	svn_revnum_t revision;
	if (id.empty()) {
		revision = SVN_INVALID_REVNUM;
	} else if (!str::stoi(id, &revision)) {
		throw PEX(std::string("Error parsing revision number ") + id);
	}

	apr_pool_t *pool = svn_pool_create(d->pool);
	svn_stringbuf_t *buf = svn_stringbuf_create("", pool);
	svn_stream_t *stream = svn_stream_from_stringbuf(buf, pool);

	svn_error_t *err = svn_ra_get_file(d->ra, path.c_str(), revision, stream, NULL, NULL, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	// Subversion 1.6 offers this in svn_string_from_stream(), but for 1.5 we
	// must copy the string manually.
	svn_stringbuf_t *work = svn_stringbuf_create("", pool);
	char *buffer = (char *)apr_palloc(pool, SVN__STREAM_CHUNK_SIZE);
	while (1) {
		apr_size_t len = SVN__STREAM_CHUNK_SIZE;
		if ((err = svn_stream_read(stream, buffer, &len))) {
			throw PEX(SvnConnection::strerr(err));
		}
		svn_stringbuf_appendbytes(work, buffer, len);
		if (len < SVN__STREAM_CHUNK_SIZE) {
			break;
		}
	}    

	if ((err = svn_stream_close(stream))) {
		throw PEX(SvnConnection::strerr(err));
	}

	std::string content = std::string(work->data, work->len);
	svn_pool_destroy(pool);
	return content;
}

// Returns a log iterator for the given branch
Backend::LogIterator *SubversionBackend::iterator(const std::string &branch, int64_t start, int64_t end)
{
	// Check if the branch exists
	apr_pool_t *pool = svn_pool_create(d->pool);
	std::string prefix = this->prefix(branch, pool);;
	svn_dirent_t *dirent;
	svn_error_t *err = svn_ra_stat(d->ra, svn_path_join(d->prefix, prefix.c_str(), pool), SVN_INVALID_REVNUM, &dirent, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}
	if (dirent == NULL) {
		if (prefix == "trunk") {
			prefix.clear();
		} else {
			throw PEX(str::printf("No such branch: %s", branch.c_str()));
		}
	}

	svn_revnum_t startrev = 0, endrev;
	apr_time_t time;
	if (start >= 0) {
		apr_time_ansi_put(&time, start);
		if ((err = svn_ra_get_dated_revision(d->ra, &startrev, time, pool)) != NULL) {
			throw PEX(SvnConnection::strerr(err));
		}
		// startrev has been set to the HEAD revision at the given start time, but
		// we're interested in all revisions after this date.
		++startrev;
	}
	if (end >= 0) {
		apr_time_ansi_put(&time, end);
		if ((err = svn_ra_get_dated_revision(d->ra, &endrev, time, pool)) != NULL) {
			throw PEX(SvnConnection::strerr(err));
		}
	} else {
		str::stoi(head(branch), &endrev);
	}

	PDEBUG << "Revision range: [ " << start << " : " << end << "] -> [" << startrev << " : " << endrev << "]" << endl;

	svn_pool_destroy(pool);
	return new SvnLogIterator(this, prefix, startrev, endrev);
}

// Adds the given revision IDs to the diffstat scheduler
void SubversionBackend::prefetch(const std::vector<std::string> &ids)
{
	if (m_prefetcher == NULL) {
		std::string numthreads = m_opts.value("threads", "10");
		int nthreads = 10;
		if (!str::stoi(numthreads, &nthreads)) {
			throw PEX(std::string("Expected number for --threads parameter: ") + numthreads);
		}
		if (nthreads == 0) {
			return;
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

// Returns the prefix for the given branch
std::string SubversionBackend::prefix(const std::string &branch, apr_pool_t *pool)
{
	std::string p;
	if (branch == "trunk") {
		p = m_opts.value("trunk", "trunk");
	} else if (!branch.empty()) {
		p = m_opts.value("branches", "branches");
		p += "/";
		p += branch;
	}

	PDEBUG << "Iterator requested for branch " << branch << " -> prefix = " << p << endl;
	return std::string(svn_path_canonicalize(p.c_str(), pool));
}

// Returns the revision data for the given ID
Revision *SubversionBackend::revision(const std::string &id)
{
	std::map<std::string, std::string> data;
	std::string rev = str::split(id, ":").back();

	svn_revnum_t revnum;
	if (!str::stoi(rev, &(revnum))) {
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
