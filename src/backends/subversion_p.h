/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: subversion_p.h
 * Internal classes for the subversion backend (interface)
 */


#ifndef SUBVERSION_BACKEND_P_H
#define SUBVERSION_BACKEND_P_H


extern apr_pool_t *global_pool;


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


// Diffstat generator thread
class SvnDiffstatGenerator : public sys::thread::Thread
{
public:
	SvnDiffstatGenerator(const std::string &url, const Options::AuthData &auth)
		: stat(NULL), d(new SvnConnection())
	{
		m_url = url, m_auth = auth;
		d->open(m_url, m_auth);
	}

	~SvnDiffstatGenerator()
	{
		delete d;
	}

	void start(const std::string &id, Diffstat *stat)
	{
		this->id = id;
		this->stat = stat;
		sys::thread::Thread::start();
	}

protected:
	void run()
	{
		// First, produce diff of previous revision and this one and save it in a
		// temporary file
		apr_pool_t *pool = svn_pool_create(d->pool);

		svn_opt_revision_t rev1, rev2;
		rev1.kind = rev2.kind = svn_opt_revision_number;
		utils::str2int(id, &(rev2.value.number));
		if (rev2.value.number <= 0) {
			svn_pool_destroy(pool);
			return;
		}
		rev1.value.number = rev2.value.number - 1;

		apr_file_t *infile = NULL, *outfile = NULL, *errfile = NULL;
		apr_file_open_stderr(&errfile, pool);
		if (apr_file_pipe_create_ex(&infile, &outfile, APR_FULL_BLOCK, pool) != APR_SUCCESS) {
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
		*stat = parser.stat();
		apr_file_close(infile);
		svn_pool_destroy(pool);
	}

public:
	std::string id;
	Diffstat *stat;
	
private:
	std::string m_url;
	Options::AuthData m_auth;
	SvnConnection *d;
};


// Diffstat scheduler
class SvnDiffstatScheduler : public sys::thread::Thread
{
public:
	SvnDiffstatScheduler(const std::string &url, const Options::AuthData &auth, const std::vector<std::string> &ids, int n = 25)
	{
		if ((size_t)n > ids.size()) {
			n = ids.size();
		}
		for (int i = 0; i < n; i++) {
			m_gen.push_back(new SvnDiffstatGenerator(url, auth));
		}
		int64_t r;
		for (unsigned int i = 0; i < ids.size(); i++) {
			utils::str2int(ids[i], &r);
			m_stats[r] = Diffstat();
			m_fetched[ids[i]] = false;
		}
	}

	~SvnDiffstatScheduler()
	{
		for (unsigned int i = 0; i < m_gen.size(); i++) {
			delete m_gen[i];
		}
	}

	Diffstat stat(const std::string &id)
	{
		m_mutex.lock();
		while (!m_fetched[id]) {
			m_mutex.unlock();
			msleep(50);
			m_mutex.lock();
		}
		int64_t r;
		utils::str2int(id, &r);
		Diffstat s = m_stats[r];
		m_mutex.unlock();
		return s;
	}

protected:
	void run()
	{
		// TODO: Not efficient
		unsigned int j = 0;
		for (std::map<int64_t, Diffstat>::iterator it = m_stats.begin(); it != m_stats.end(); ++it) {
			m_gen[j]->wait();
			if (m_gen[j]->stat != NULL) {
				m_mutex.lock();
				m_fetched[m_gen[j]->id] = true;
				m_gen[j]->stat = NULL;
				m_mutex.unlock();
			}
			m_gen[j]->start(utils::int2str(it->first), &(it->second));
			j = (j+1) % m_gen.size();
		}
		for (unsigned int i = 0; i < m_gen.size(); i++) {
			m_gen[i]->wait();
			if (m_gen[i]->stat != NULL) {
				m_mutex.lock();
				m_fetched[m_gen[i]->id] = true;
				m_gen[i]->stat = NULL;
				m_mutex.unlock();
			}
		}
	}

private:
	std::vector<SvnDiffstatGenerator *> m_gen;
	std::map<int64_t, Diffstat> m_stats;
	std::map<std::string, bool> m_fetched;
	std::string m_url;
	Options::AuthData m_auth;
	sys::thread::Mutex m_mutex;
};


#endif // SUBVERSION_BACKEND_P_H
