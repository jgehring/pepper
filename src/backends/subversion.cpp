/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: subversion.cpp
 * Subversion repository backend
 */


#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include <svn_client.h>
#include <svn_cmdline.h>
#include <svn_fs.h>
#include <svn_path.h>
#include <svn_pools.h>
#include <svn_ra.h>
#include <svn_time.h>
#include <svn_utf.h>

#include "main.h"
#include "options.h"
#include "revision.h"
#include "utils.h"

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
	void open(const std::string &url, const Options::AuthData &auth)
	{
		pool = svn_pool_create(NULL);
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


// Subversion callback for log messages
static svn_error_t *logReceiver(void *baton, svn_log_entry_t *entry, apr_pool_t *pool) 
{
	std::vector<std::string> *dest = static_cast<std::vector<std::string> *>(baton);
	dest->push_back(Utils::int2str(entry->revision));
	return SVN_NO_ERROR;
}

// Constructor
SubversionBackend::SubversionRevisionIterator::SubversionRevisionIterator(SvnConnection *c, const std::string &prefix, long int head)
	: Backend::RevisionIterator()
{
	// Determine revisions
	apr_pool_t *pool = svn_pool_create(c->pool);
	apr_array_header_t *path = apr_array_make(pool, 1, sizeof (const char *));
	APR_ARRAY_PUSH(path, const char *) = svn_path_canonicalize(prefix.empty() ? "." : prefix.c_str(), pool);
	apr_array_header_t *props = apr_array_make(pool, 1, sizeof (const char *)); // Intentionally empty

//	svn_error_t *err = svn_ra_get_log2(c->ra, path, 0, head, 0, FALSE, TRUE, FALSE, props, &logReceiver, &m_ids, pool);
	svn_error_t *err = svn_ra_get_log2(c->ra, path, 0, head, 0, FALSE, FALSE /* otherwise, copy history will be ignored */, FALSE, props, &logReceiver, &m_ids, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	svn_pool_clear(pool);
}


// Constructor
SubversionBackend::SubversionBackend(const Options &options)
	: Backend(options), d(new SvnConnection())
{

}

// Destructor
SubversionBackend::~SubversionBackend()
{
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

	std::string url = m_opts.repoUrl();
	if (!url.compare(0, 1, "/")) {
		url = std::string("file://") + url;
	}
	d->open(url, m_opts.authData());
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
	svn_revnum_t rev;
	svn_error_t *err = svn_ra_get_latest_revnum(d->ra, &rev, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}
	svn_pool_destroy(pool);
	return Utils::int2str(rev);
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
	// First, produce diff of previous revision and this one and save it in a
	// temporary file
	apr_pool_t *pool = svn_pool_create(d->pool);

	svn_opt_revision_t rev1, rev2;
	rev1.kind = rev2.kind = svn_opt_revision_number;
	Utils::str2int(id, &(rev2.value.number));
	if (rev2.value.number <= 0) {
		svn_pool_destroy(pool);
		return Diffstat();
	}
	rev1.value.number = rev2.value.number - 1;

	apr_file_t *outfile = NULL, *errfile = NULL;
	apr_file_open_stderr(&errfile, pool);

	const char *tmpdir = NULL;
	if (apr_temp_dir_get(&tmpdir, pool) != APR_SUCCESS) {
		throw PEX("Unable to determine temporary directory");
	}
	char *templ = apr_psprintf(pool, "%s/XXXXXX", tmpdir);
	if (apr_file_mktemp(&outfile, templ, APR_CREATE | APR_READ | APR_WRITE | APR_EXCL, pool) != APR_SUCCESS) {
		throw PEX("Unable to create temporary file");
	}

	svn_error_t *err = svn_client_diff4(apr_array_make(pool, 0, 1), d->url, &rev1, d->url, &rev2, NULL, svn_depth_infinity, FALSE, FALSE, TRUE, APR_LOCALE_CHARSET, outfile, errfile, NULL, d->ctx, pool);
	if (err != NULL) {
		throw PEX(SvnConnection::strerr(err));
	}

	// Generate the diffstat

	// TODO: Possible without closing the file?
	apr_file_close(outfile);
	std::ifstream in(templ, std::ifstream::in);
	Diffstat stat(in);

	in.close();
	apr_file_remove(templ, pool);
	svn_pool_destroy(pool);
	return stat;
}

// Returns a revision iterator for the given branch
Backend::RevisionIterator *SubversionBackend::iterator(const std::string &branch)
{
	std::string prefix;
	if (branch == "trunk") {
		prefix = branch;
	} else if (!branch.empty()) {
		prefix = "branches/";
		prefix += branch;
	}

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
			throw PEX(Utils::strprintf("No such branch: %s", branch.c_str()));
		}
	}
	svn_pool_destroy(pool);

	long int headrev;
	Utils::str2int(head(prefix), &headrev);
	return new SubversionRevisionIterator(d, prefix, headrev);
}

// Returns the revision data for the given ID
Revision *SubversionBackend::revision(const std::string &id)
{
	std::map<std::string, std::string> data;

	apr_pool_t *pool = svn_pool_create(d->pool);
	apr_hash_t *props;
	svn_revnum_t revnum;
	Utils::str2int(id, &(revnum));

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
