/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: subversion.cpp
 * Subversion repository backend
 */


#define __STDC_CONSTANT_MACROS // For INT64_C

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include <svn_client.h>
#include <svn_cmdline.h>
#include <svn_fs.h>
#include <svn_path.h>
#include <svn_pools.h>
#include <svn_ra.h>
#include <svn_time.h>
#include <svn_utf.h>

#include "main.h"

#include "jobqueue.h"
#include "logger.h"
#include "options.h"
#include "revision.h"
#include "utils.h"

#include "syslib/fs.h"
#include "syslib/parallel.h"

#include "backends/subversion.h"


// Private Subversion repository connection class
class SvnConnection
{
public:
	SvnConnection()
		: pool(NULL), ctx(NULL), ra(NULL), url(NULL)
	{
	}

	~SvnConnection()
	{
		if (pool) {
			svn_pool_destroy(pool);
		}
	}

	// Opens the connection to the Subversion repository
	void open(const std::string &url, const Options::AuthData &auth, apr_pool_t *ppool = NULL)
	{
		PTRACE << "Opening connection to " << url << endl;

		pool = svn_pool_create(ppool);
		init();

		// Canoicalize the repository URL
		this->url = svn_path_uri_encode(svn_path_canonicalize(url.c_str(), pool), pool);

		// Create the client context
		svn_error_t *err;
		if ((err = svn_client_create_context(&ctx, pool))) {
			throw PEX(strerr(err));
		}
		if ((err = svn_config_get_config(&(ctx->config), NULL, pool))) {
			throw PEX(strerr(err));
		}

		// Setup the authentication data
		svn_auth_baton_t *auth_baton;
		svn_config_t *config = hashget<svn_config_t *>(ctx->config, SVN_CONFIG_CATEGORY_CONFIG);
		if ((err = svn_cmdline_setup_auth_baton(&auth_baton, /*(session->flags & SF_NON_INTERACTIVE)*/0, auth.username.c_str(), auth.password.c_str(), NULL, /*(session->flags & SF_NO_AUTH_CACHE)*/0, config, NULL, NULL, pool))) {
			throw PEX(strerr(err));
		}
		ctx->auth_baton = auth_baton;

		/* Setup the RA session */
		if ((err = svn_client_open_ra_session(&ra, this->url, ctx, pool))) {
			throw PEX(strerr(err));
		}
	}

	static std::string strerr(svn_error_t *err)
	{
		// Similar to svn_handle_error2(), but returns the error description as a std::string
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

private:
	void init()
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

	template <typename T>
	static T hashget(apr_hash_t *hash, const char *key)
	{
		return static_cast<T>(apr_hash_get(hash, key, APR_HASH_KEY_STRING));
	}

public:
	apr_pool_t *pool;
	svn_client_ctx_t *ctx;
	svn_ra_session_t *ra;
	const char *url;
};


// streambuf implementation for apr_file_t
class AprStreambuf : public std::streambuf
{
public:
	explicit AprStreambuf(apr_file_t *file) 
	   : std::streambuf(), f(file),
	     m_putback(8), m_buffer(4096 + 8)
	{
		char *end = &m_buffer.front() + m_buffer.size();
		setg(end, end, end);
	}

private:
	int_type underflow()
	{
		if (gptr() < egptr()) // buffer not exhausted
			return traits_type::to_int_type(*gptr());

		char *base = &m_buffer.front();
		char *start = base;

		if (eback() == base) // true when this isn't the first fill
		{
			// Make arrangements for putback characters
			memmove(base, egptr() - m_putback, m_putback);
			start += m_putback;
		}

		// start is now the start of the buffer, proper.
		// Read from fptr_ in to the provided buffer
		size_t n = m_buffer.size() - (start - base);
		apr_file_read(f, start, &n);
		if (n == 0)
			return traits_type::eof();

		// Set buffer pointers
		setg(base, start, start + n);

		return traits_type::to_int_type(*gptr());
	}

private:
	apr_file_t *f;
	const std::size_t m_putback;
	std::vector<char> m_buffer;

private:
	// Not allowed
	AprStreambuf(const AprStreambuf &);
	AprStreambuf &operator=(const AprStreambuf &);
};


// Diffstat generator thread
class SvnDiffstatThread : public sys::parallel::Thread
{
public:
	SvnDiffstatThread(const std::string &url, const Options::AuthData &auth, JobQueue<svn_revnum_t, Diffstat> *queue)
		: d(new SvnConnection()), m_queue(queue)
	{
		d->open(url, auth);
	}

	~SvnDiffstatThread()
	{
		delete d;
	}

protected:
	void run()
	{
		apr_pool_t *pool = svn_pool_create(d->pool);
		svn_opt_revision_t rev1, rev2;
		rev1.kind = rev2.kind = svn_opt_revision_number;

		svn_revnum_t revision;
		while (m_queue->getArg(&revision)) {
			apr_pool_t *subpool = svn_pool_create(pool);

			rev2.value.number = revision;
			rev1.value.number = rev2.value.number - 1;
			if (rev2.value.number <= 0) {
				m_queue->done(revision, Diffstat());
				continue;
			}

			apr_file_t *infile = NULL, *outfile = NULL, *errfile = NULL;
			apr_file_open_stderr(&errfile, subpool);
			if (apr_file_pipe_create(&infile, &outfile, subpool) != APR_SUCCESS) {
				throw PEX("Unable to create pipe");
			}

			AprStreambuf buf(infile);
			std::istream in(&buf);
			DiffParser parser(in);
			parser.start();

			svn_error_t *err = svn_client_diff4(apr_array_make(subpool, 0, 1), d->url, &rev1, d->url, &rev2, NULL, svn_depth_infinity, FALSE, FALSE, TRUE, APR_LOCALE_CHARSET, outfile, errfile, NULL, d->ctx, subpool);
			if (err != NULL) {
				throw PEX(SvnConnection::strerr(err));
			}

			apr_file_close(outfile);
			parser.wait();
			m_queue->done(revision, parser.stat());
			apr_file_close(infile);

			svn_pool_destroy(subpool);
		}
		svn_pool_destroy(pool);
	}

private:
	SvnConnection *d;
	JobQueue<svn_revnum_t, Diffstat> *m_queue;
};


// Handles the prefetching of diffstats
class SvnDiffstatPrefetcher
{
public:
	SvnDiffstatPrefetcher(const std::string &url, const Options::AuthData &auth, int n = 4)
	{
		for (int i = 0; i < n; i++) {
			SvnDiffstatThread * thread = new SvnDiffstatThread(url, auth, &m_queue);
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
		std::vector<svn_revnum_t> revnums;
		svn_revnum_t rev;
		for (unsigned int i = 0; i < revisions.size(); i++) {
			if (utils::str2int(revisions[i], &rev)) {
				revnums.push_back(rev);
			}
		}
		m_queue.put(revnums);
	}

	bool get(const std::string &revision, Diffstat *dest)
	{
		svn_revnum_t rev;
		if (utils::str2int(revision, &rev)) {
			return m_queue.getResult(rev, dest);
		}
		return false;
	}

	bool willFetch(const std::string &revision)
	{
		svn_revnum_t rev;
		if (utils::str2int(revision, &rev)) {
			return m_queue.hasArg(rev);
		}
		return false;
	}

private:
	JobQueue<svn_revnum_t, Diffstat> m_queue;
	std::vector<SvnDiffstatThread *> m_threads;
};


// Constructor
SubversionBackend::SvnLogIterator::SvnLogIterator(const std::string &url, const std::string &prefix, const Options::AuthData &auth, svn_revnum_t head)
	: Backend::LogIterator(), d(new SvnConnection()), m_prefix(prefix), m_head(head), m_index(0), m_finished(false)
{
	d->open(url, auth);
}

// Desctructor
SubversionBackend::SvnLogIterator::~SvnLogIterator()
{
	delete d;
}

// Returns the next revision IDs, or an empty vector
std::vector<std::string> SubversionBackend::SvnLogIterator::nextIds()
{
	m_mutex.lock();
	while (m_index >= m_ids.size() && !m_finished) {
		m_cond.wait(&m_mutex);
	}

	std::vector<std::string> next;
	while (m_index < m_ids.size()) {
		next.push_back(m_ids[m_index++]);
	}
	m_mutex.unlock();
	return next;
}

struct logReceiverBaton
{
	sys::parallel::Mutex *mutex;
	sys::parallel::WaitCondition *cond;
	std::vector<std::string> temp;
	std::vector<std::string> *ids;
};

// Subversion callback for log messages
static svn_error_t *logReceiver(void *baton, svn_log_entry_t *entry, apr_pool_t *) 
{
	logReceiverBaton *b = static_cast<logReceiverBaton *>(baton);
	b->temp.push_back(utils::int2str(entry->revision));

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
	sys::parallel::MutexLocker locker(&m_mutex);

	apr_pool_t *pool = svn_pool_create(d->pool);
	apr_pool_t *subpool = svn_pool_create(pool);
	apr_array_header_t *path = apr_array_make(pool, 1, sizeof (const char *));
	APR_ARRAY_PUSH(path, const char *) = svn_path_canonicalize(m_prefix.empty() ? "." : m_prefix.c_str(), pool);
	apr_array_header_t *props = apr_array_make(pool, 1, sizeof (const char *)); // Intentionally empty
	int windowSize = 512;
	if (!strncmp(d->url, "file://", strlen("file://"))) {
		windowSize = 0;
	}

	logReceiverBaton baton;
	baton.mutex = &m_mutex;
	baton.cond = &m_cond;
	baton.ids = &m_ids;

	// Determine revisions, but not at once
	int64_t start = 0;
	while (start < m_head-1) {
		locker.unlock();

		//	svn_error_t *err = svn_ra_get_log2(c->ra, path, 0, m_head, windowSize, FALSE, TRUE, FALSE, props, &logReceiver, &m_ids, pool);
		svn_error_t *err = svn_ra_get_log2(d->ra, path, start, m_head, windowSize, FALSE, FALSE /* otherwise, copy history will be ignored */, FALSE, props, &logReceiver, &baton, pool);
		if (err != NULL) {
			throw PEX(SvnConnection::strerr(err));
		}

		start = (windowSize > 0 ? start + windowSize : m_head);
		svn_pool_clear(subpool);
	}

	locker.relock();
	for (unsigned int i = 0; i < baton.temp.size(); i++) {
		m_ids.push_back(baton.temp[i]);
	}

	m_finished = true;
	m_cond.wakeAll();
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

	std::string url = m_opts.repoUrl();
	if (!url.compare(0, 1, "/")) {
		url = std::string("file://") + url;
	}
	d->open(url, m_opts.authData());
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
	// TODO: Respect the branch argument
	apr_pool_t *pool = svn_pool_create(d->pool);
	svn_revnum_t rev;
	svn_error_t *err = svn_ra_get_latest_revnum(d->ra, &rev, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}
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
	// Assume the repository has a standard layout and branches can be found
	// in "/branches/", if any
	std::vector<std::string> branches;

	apr_pool_t *pool = svn_pool_create(d->pool);

	// Check if branches directoy is present
	svn_dirent_t *dirent;
	svn_error_t *err = svn_ra_stat(d->ra, "branches", SVN_INVALID_REVNUM, &dirent, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	if (dirent == NULL || dirent->kind != svn_node_dir) {
		// It's not here; let's call the main branch "trunk"
		branches.push_back("trunk");
		return branches;
	}

	// Get directory entries
	apr_hash_t *dirents = apr_hash_make(pool), *props = apr_hash_make(pool);
	err = svn_ra_get_dir2(d->ra, &dirents, NULL, &props, "branches", SVN_INVALID_REVNUM, SVN_DIRENT_KIND, pool);
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

// Returns a diffstat for the specified revision
Diffstat SubversionBackend::diffstat(const std::string &id)
{
	if (m_prefetcher && m_prefetcher->willFetch(id)) {
		PDEBUG << "Revision " << id << " will be prefetched" << endl;
		Diffstat stat;
		if (!m_prefetcher->get(id, &stat)) {
			throw PEX(utils::strprintf("Failed to retrieve diffstat for revision %s", id.c_str()));
		}
		return stat;
	}

	PDEBUG << "Fetching revision " << id << " manually" << endl;

	// First, produce diff of previous revision and this one and save it in a
	// temporary file
	apr_pool_t *pool = svn_pool_create(d->pool);

	svn_opt_revision_t rev1, rev2;
	rev1.kind = rev2.kind = svn_opt_revision_number;
	utils::str2int(id, (long int *)&(rev2.value.number));
	if (rev2.value.number <= 0) {
		svn_pool_destroy(pool);
		return Diffstat();
	}
	rev1.value.number = rev2.value.number - 1;

	apr_file_t *infile = NULL, *outfile = NULL, *errfile = NULL;
	apr_file_open_stderr(&errfile, pool);
	if (apr_file_pipe_create(&infile, &outfile, pool) != APR_SUCCESS) {
		throw PEX("Unable to create pipe");
	}

	AprStreambuf buf(infile);
	std::istream in(&buf);
	DiffParser parser(in);
	parser.start();

	svn_error_t *err = svn_client_diff4(apr_array_make(pool, 0, 1), d->url, &rev1, d->url, &rev2, NULL, svn_depth_infinity, FALSE, FALSE, TRUE, APR_LOCALE_CHARSET, outfile, errfile, NULL, d->ctx, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	apr_file_close(outfile);
	parser.wait();
	apr_file_close(infile);
	svn_pool_destroy(pool);
	return parser.stat();
}

// Returns a log iterator for the given branch
Backend::LogIterator *SubversionBackend::iterator(const std::string &branch)
{
	std::string prefix;
	if (branch == "trunk") {
		prefix = branch;
	} else if (!branch.empty()) {
		prefix = "branches/";
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
	svn_pool_destroy(pool);

	long int headrev;
	utils::str2int(head(prefix), &headrev);
	return new SvnLogIterator(d->url, prefix, m_opts.authData(), headrev);
}

// Adds the given revision IDs to the diffstat scheduler
void SubversionBackend::prefetch(const std::vector<std::string> &ids)
{
	if (m_prefetcher == NULL) {
		m_prefetcher = new SvnDiffstatPrefetcher(d->url, m_opts.authData(), 10);
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

// Returns the revision data for the given ID
Revision *SubversionBackend::revision(const std::string &id)
{
	std::map<std::string, std::string> data;

	apr_pool_t *pool = svn_pool_create(d->pool);
	apr_hash_t *props;
	svn_revnum_t revnum;
	utils::str2int(id, &(revnum));

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
