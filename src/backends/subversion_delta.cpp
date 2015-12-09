/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-present Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: subversion_delta.cpp
 * Diff retrieval for the subversion repository backend
 */


#define __STDC_CONSTANT_MACROS // For INT64_C

#include "main.h"

#include <svn_delta.h>
#include <svn_path.h>
#include <svn_pools.h>
#include <svn_props.h>
#include <svn_utf.h>

#include "jobqueue.h"
#include "logger.h"
#include "strlib.h"

#include "backends/subversion_p.h"


// Statement macro for checking APR calls, similar to SVN_ERR
#define APR_ERR(expr) \
	do { \
		apr_status_t apr_status__temp = (expr); \
		if (apr_status__temp) \
			return svn_error_wrap_apr(apr_status__temp, NULL); \
	} while (0)


// streambuf implementation for apr_file_t
class AprStreambuf : public std::streambuf
{
public:
	explicit AprStreambuf(apr_file_t *file) 
		: std::streambuf(), f(file), m_putback(8), m_buffer(4096 + 8)
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
		apr_status_t status = apr_file_read(f, start, &n);
		if (n == 0 || status == APR_EOF)
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


// Extra namespace for local structures
namespace SvnDelta
{

// Baton for delta editor
struct Baton
{
	const char *target;
	apr_file_t *out;

	svn_ra_session_t *ra;
	svn_revnum_t revision;
	svn_revnum_t base_revision;
	svn_revnum_t target_revision;
	apr_hash_t *deleted_paths;

	const char *tempdir;
	const char *empty_file;

	apr_pool_t *pool;

	static Baton *make(svn_revnum_t baserev, svn_revnum_t rev, apr_file_t *outfile, apr_pool_t *pool)
	{
		Baton *baton = (Baton *)apr_pcalloc(pool, sizeof(Baton));

		baton->target = "";
		baton->out = outfile;
		baton->base_revision = baserev;
		baton->revision = rev;
		baton->deleted_paths = apr_hash_make(pool);
		svn_io_temp_dir(&(baton->tempdir), pool);
		baton->empty_file = NULL; // TODO
		baton->pool = pool;
		return baton;
	}
};

struct DirBaton
{
	const char *path;
	DirBaton *dir_baton;

	Baton *edit_baton;
	apr_pool_t *pool;

	static DirBaton *make(const char *path, DirBaton *parent_baton, Baton *edit_baton, apr_pool_t *pool)
	{
		DirBaton *dir_baton = (DirBaton *)apr_pcalloc(pool, sizeof(DirBaton));

		dir_baton->dir_baton = parent_baton;
		dir_baton->edit_baton = edit_baton;
		dir_baton->pool = pool;
		dir_baton->path = apr_pstrdup(pool, path);
		return dir_baton;
	}
};

struct FileBaton
{
	const char *path;
	const char *path_start_revision;
	apr_file_t *file_start_revision;
	apr_hash_t *pristine_props;
	apr_array_header_t *propchanges;
	const char *path_end_revision;
	apr_file_t *file_end_revision;
	svn_txdelta_window_handler_t apply_handler;
	void *apply_baton;

	Baton *edit_baton;
	apr_pool_t *pool;

	static FileBaton *make(const char *path, void *edit_baton, apr_pool_t *pool)
	{
		FileBaton *file_baton = (FileBaton *)apr_pcalloc(pool, sizeof(FileBaton));
		Baton *eb = static_cast<Baton *>(edit_baton);

		file_baton->edit_baton = eb;
		file_baton->pool = pool;
		file_baton->path = apr_pstrdup(pool, path);
		file_baton->propchanges  = apr_array_make(pool, 1, sizeof(svn_prop_t));
		return file_baton;
	}
};


// Utility funtions
svn_error_t *open_tempfile(apr_file_t **file, const char **path, FileBaton *b)
{
	const char *temp = svn_path_join(b->edit_baton->tempdir, "tempfile", b->pool);
	return svn_io_open_unique_file2(file, path, temp, ".tmp", svn_io_file_del_on_pool_cleanup, b->pool);
}

svn_error_t *get_file_from_ra(FileBaton *b, svn_revnum_t revision)
{
	PTRACE << b->path << "@" << revision << endl;
	svn_stream_t *fstream;
	apr_file_t *file;

	SVN_ERR(open_tempfile(&file, &(b->path_start_revision), b));
	fstream = svn_stream_from_aprfile2(file, FALSE, b->pool);
	SVN_ERR(svn_ra_get_file(b->edit_baton->ra, b->path, revision, fstream, NULL, &(b->pristine_props), b->pool));
	return svn_stream_close(fstream);
}


// Delta editor callback functions
svn_error_t *set_target_revision(void *edit_baton, svn_revnum_t target_revision, apr_pool_t * /*pool*/)
{
	Baton *eb = static_cast<Baton *>(edit_baton);
	eb->target_revision = target_revision;
	return SVN_NO_ERROR;
}

svn_error_t *open_root(void *edit_baton, svn_revnum_t /*base_revision*/, apr_pool_t *pool, void **root_baton)
{
	PTRACE << endl;

	Baton *eb = static_cast<Baton *>(edit_baton);
	DirBaton *b = DirBaton::make("", NULL, eb, pool);

	*root_baton = b;
	return SVN_NO_ERROR;
}

// Prototype
svn_error_t *close_file(void *file_baton, const char *text_checksum, apr_pool_t *pool);

svn_error_t *delete_entry(const char *path, svn_revnum_t target_revision, void *parent_baton, apr_pool_t *pool)
{
	DirBaton *pb = static_cast<DirBaton *>(parent_baton);
	Baton *eb = pb->edit_baton;

	// File or directory?
	svn_dirent_t *dirent;
	SVN_ERR(svn_ra_stat(eb->ra, path, eb->base_revision, &dirent, pool));
	if (dirent == NULL) {
		PDEBUG << "Error: Unable to stat " << path << "@" << eb->base_revision << endl;
		return SVN_NO_ERROR;
	}

	PTRACE << path << "@" << eb->base_revision <<" is a " << (dirent->kind == svn_node_dir ? "directory" : "file") << endl;

	if (dirent->kind == svn_node_file) {
		FileBaton *b = FileBaton::make(path, eb, pool);
		SVN_ERR(get_file_from_ra(b, eb->base_revision));
		SVN_ERR(open_tempfile(&(b->file_end_revision), &(b->path_end_revision), b));
		SVN_ERR(close_file(b, "", pool));
	} else {
		PTRACE << "Listing " << path << "@" << eb->base_revision << endl;
		apr_hash_t *dirents;
		apr_pool_t *iterpool = svn_pool_create(pool);

		SVN_ERR(svn_ra_get_dir2(eb->ra, &dirents, NULL, NULL, path, eb->base_revision, 0, pool));
		// "Delete" directory recursively
		for (apr_hash_index_t *hi = apr_hash_first(pool, dirents); hi; hi = apr_hash_next(hi)) {
			svn_pool_clear(iterpool);

			const char *entry;
			svn_dirent_t *dirent;
			apr_hash_this(hi, (const void **)(void *)&entry, NULL, (void **)(void *)&dirent);
			SVN_ERR(delete_entry((const char *)svn_path_join(path, entry, pool), target_revision, parent_baton, iterpool));
		}

		svn_pool_destroy(iterpool);
	}

	return SVN_NO_ERROR;
}

svn_error_t *add_directory(const char *path, void *parent_baton, const char *, svn_revnum_t, apr_pool_t *pool, void **child_baton)
{
	PTRACE << path << endl;
	DirBaton *pb = static_cast<DirBaton *>(parent_baton);

	DirBaton *b = DirBaton::make(path, pb, pb->edit_baton, pool);
	*child_baton = b;

	return SVN_NO_ERROR;
}

svn_error_t *open_directory(const char *path, void *parent_baton, svn_revnum_t /*base_revision*/, apr_pool_t *pool, void **child_baton)
{
	PTRACE << path << endl;
	DirBaton *pb = static_cast<DirBaton *>(parent_baton);

	DirBaton *b = DirBaton::make(path, pb, pb->edit_baton, pool);
	*child_baton = b;

	return SVN_NO_ERROR;
}

svn_error_t *add_file(const char *path, void *parent_baton, const char * /*copyfrom_path*/, svn_revnum_t /*copyfrom_revision*/, apr_pool_t *pool, void **file_baton)
{
	PTRACE << path << endl;
	DirBaton *db = static_cast<DirBaton *>(parent_baton);
	FileBaton *b = FileBaton::make(path, db->edit_baton, pool);
	*file_baton = b;

	b->pristine_props = apr_hash_make(pool);
	return open_tempfile(&(b->file_start_revision), &(b->path_start_revision), b);
}

svn_error_t *open_file(const char *path, void *parent_baton, svn_revnum_t base_revision, apr_pool_t *pool, void **file_baton)
{
	PTRACE << path << ", base = " << base_revision << endl;
	DirBaton *db = static_cast<DirBaton *>(parent_baton);
	FileBaton *b = FileBaton::make(path, db->edit_baton, pool);
	*file_baton = b;

	// TODO: No need to get the whole file if it is binary...
	return get_file_from_ra(b, base_revision);
}

svn_error_t *window_handler(svn_txdelta_window_t *window, void *window_baton)
{
	FileBaton *b = static_cast<FileBaton *>(window_baton);
	SVN_ERR(b->apply_handler(window, b->apply_baton));
	if (!window) {
		SVN_ERR(svn_io_file_close(b->file_start_revision, b->pool));
		SVN_ERR(svn_io_file_close(b->file_end_revision, b->pool));
	}
	return SVN_NO_ERROR;
}

svn_error_t *apply_textdelta(void *file_baton, const char * /*base_checksum*/, apr_pool_t * /*pool*/, svn_txdelta_window_handler_t *handler, void **handler_baton)
{
	FileBaton *b = static_cast<FileBaton *>(file_baton);
	PTRACE << "base is " << b->path_start_revision << endl;
	SVN_ERR(svn_io_file_open(&(b->file_start_revision), b->path_start_revision, APR_READ, APR_OS_DEFAULT, b->pool));

	SVN_ERR(open_tempfile(&(b->file_end_revision), &(b->path_end_revision), b));

	svn_txdelta_apply(svn_stream_from_aprfile2(b->file_start_revision, TRUE, b->pool), svn_stream_from_aprfile2(b->file_end_revision, TRUE, b->pool), NULL, b->path, b->pool, &(b->apply_handler), &(b->apply_baton));
	*handler = window_handler;
	*handler_baton = file_baton;
	return SVN_NO_ERROR;
}

svn_error_t *close_file(void *file_baton, const char * /*text_checksum*/, apr_pool_t * /*pool*/)
{
	FileBaton *b = static_cast<FileBaton *>(file_baton);
	Baton *eb = b->edit_baton;

	if (b->path_start_revision == NULL || b->path_end_revision == NULL) {
		PDEBUG << b->path << "@" << eb->target_revision << " Insufficient diff data (nothing has changed)" << endl;
		return SVN_NO_ERROR;
	}

	PTRACE << b->path_start_revision << " -> " << b->path_end_revision << endl;

	// Skip binary diffs
	const char *mimetype1 = NULL, *mimetype2 = NULL;
	if (b->pristine_props) {
		svn_string_t *pristine_val;
		pristine_val = (svn_string_t *)apr_hash_get(b->pristine_props, SVN_PROP_MIME_TYPE, strlen(SVN_PROP_MIME_TYPE));
		if (pristine_val) {
			mimetype1 = pristine_val->data;
		}
	}
	if (b->propchanges) {
		int i;
		svn_prop_t *propchange;
		for (i = 0; i < b->propchanges->nelts; i++) {
			propchange = &APR_ARRAY_IDX(b->propchanges, i, svn_prop_t);
			if (strcmp(propchange->name, SVN_PROP_MIME_TYPE) == 0) {
				if (propchange->value) {
					mimetype2 = propchange->value->data;
				}
				break;
			}
		}
	}

	// TODO: Proper handling of mime-type changes
	if ((mimetype1 && svn_mime_type_is_binary(mimetype1)) || (mimetype2 && svn_mime_type_is_binary(mimetype2))) {
		PDEBUG << "Skipping binary files" << endl;
		return SVN_NO_ERROR;
	}

	// Finally, perform the diff
	static const char equal_string[] =
		"===================================================================";

	svn_diff_t *diff;
	svn_stream_t *os;
	os = svn_stream_from_aprfile2(eb->out, TRUE, b->pool);
	svn_diff_file_options_t *opts = svn_diff_file_options_create(b->pool);
	SVN_ERR(svn_diff_file_diff_2(&diff, b->path_start_revision, b->path_end_revision, opts, b->pool));
	// Print out the diff header
	SVN_ERR(svn_stream_printf_from_utf8(os, APR_LOCALE_CHARSET, b->pool, "Index: %s" APR_EOL_STR "%s" APR_EOL_STR, b->path, equal_string));
	// Output the actual diff
	SVN_ERR(svn_diff_file_output_unified3(os, diff, b->path_start_revision, b->path_end_revision, b->path, b->path, APR_LOCALE_CHARSET, NULL, FALSE, b->pool));

	return SVN_NO_ERROR;
}

svn_error_t *close_directory(void *, apr_pool_t *)
{
	return SVN_NO_ERROR;
}

svn_error_t *change_file_prop(void *file_baton, const char *name, const svn_string_t *value, apr_pool_t * /*pool*/)
{
	FileBaton *b = static_cast<FileBaton *>(file_baton);
	svn_prop_t *propchange = (svn_prop_t *)apr_array_push(b->propchanges);
	propchange->name = apr_pstrdup(b->pool, name);
	propchange->value = value ? svn_string_dup(value, b->pool) : NULL;
	return SVN_NO_ERROR;
}

svn_error_t *change_dir_prop(void *, const char *, const svn_string_t *, apr_pool_t *)
{
	return SVN_NO_ERROR;
}

svn_error_t *absent_directory(const char *path, void * /*parent_baton*/, apr_pool_t * /*pool*/)
{
	PDEBUG << path << endl;
	return SVN_NO_ERROR;
}

svn_error_t *absent_file(const char *path, void * /*parent_baton*/, apr_pool_t * /*pool*/)
{
	PDEBUG << path << endl;
	return SVN_NO_ERROR;
}

svn_error_t *close_edit(void * /*edit_baton*/, apr_pool_t * /*pool*/)
{
	return SVN_NO_ERROR;
}

} // namespace SvnDelta


// Constructor
SvnDiffstatThread::SvnDiffstatThread(SvnConnection *connection, JobQueue<std::string, DiffstatPtr> *queue)
	: d(new SvnConnection()), m_queue(queue)
{
	d->open(connection);
}

// Destructor
SvnDiffstatThread::~SvnDiffstatThread()
{
	delete d;
}

// This function will always perform a diff on the full repository in
// order to avoid errors due to non-existent paths and to cache consistency.
DiffstatPtr SvnDiffstatThread::diffstat(SvnConnection *c, svn_revnum_t r1, svn_revnum_t r2, apr_pool_t *pool)
{
	if (r2 <= 0) {
		return std::make_shared<Diffstat>();
	}

	svn_opt_revision_t rev1, rev2;
	rev1.kind = rev2.kind = svn_opt_revision_number;
	rev1.value.number = r1;
	rev2.value.number = r2;
	svn_error_t *err;

	apr_file_t *infile = NULL, *outfile = NULL, *errfile = NULL;
	if (apr_file_open_stderr(&errfile, pool) != APR_SUCCESS) {
		throw PEX("Unable to open stderr");
	}
	if (apr_file_pipe_create(&infile, &outfile, pool) != APR_SUCCESS) {
		throw PEX("Unable to create pipe for reading diff data");
	}

	AprStreambuf buf(infile);
	std::istream in(&buf);
	DiffParser parser(in);
	parser.start();

	PTRACE << "Fetching diffstat for revision " << r1 << ":" << r2 << endl;

	// Setup the diff editor
	apr_pool_t *subpool = svn_pool_create(pool);
	svn_delta_editor_t *editor = svn_delta_default_editor(subpool);
	SvnDelta::Baton *baton = SvnDelta::Baton::make(r1, r2, outfile, subpool);

	// Open RA session for extra calls during diff
	err = svn_client_open_ra_session(&baton->ra, c->root, c->ctx, pool);
	if (err != NULL) {
		apr_file_close(outfile);
		apr_file_close(infile);
		throw PEX(str::printf("Diffstat fetching of revision %ld:%ld failed: %s", r1, r2, SvnConnection::strerr(err).c_str()));
	}

	editor->set_target_revision = SvnDelta::set_target_revision;
	editor->open_root = SvnDelta::open_root;
	editor->delete_entry = SvnDelta::delete_entry;
	editor->add_directory = SvnDelta::add_directory;
	editor->open_directory = SvnDelta::open_directory;
	editor->add_file = SvnDelta::add_file;
	editor->open_file = SvnDelta::open_file;
	editor->apply_textdelta = SvnDelta::apply_textdelta;
	editor->close_file = SvnDelta::close_file;
	editor->close_directory = SvnDelta::close_directory;
	editor->change_file_prop = SvnDelta::change_file_prop;
	editor->change_dir_prop = SvnDelta::change_dir_prop;
	editor->close_edit = SvnDelta::close_edit;
	editor->absent_directory = SvnDelta::absent_directory;
	editor->absent_file = SvnDelta::absent_file;

	const svn_ra_reporter3_t *reporter;
	void *report_baton;
	err = svn_ra_do_diff3(c->ra, &reporter, &report_baton, rev2.value.number, "", svn_depth_infinity, TRUE, TRUE, c->root, editor, baton, pool);
	if (err != NULL) {
		apr_file_close(outfile);
		apr_file_close(infile);
		throw PEX(str::printf("Diffstat fetching of revision %ld:%ld failed: %s", r1, r2, SvnConnection::strerr(err).c_str()));
	}

	err = reporter->set_path(report_baton, "", rev1.value.number, svn_depth_infinity, FALSE, NULL, pool);
	if (err != NULL) {
		apr_file_close(outfile);
		apr_file_close(infile);
		throw PEX(str::printf("Diffstat fetching of revision %ld:%ld failed: %s", r1, r2, SvnConnection::strerr(err).c_str()));
	}

	err = reporter->finish_report(report_baton, pool);
	if (err != NULL) {
		apr_file_close(outfile);
		apr_file_close(infile);
		throw PEX(str::printf("Diffstat fetching of revision %ld:%ld failed: %s", r1, r2, SvnConnection::strerr(err).c_str()));
	}

	if (apr_file_close(outfile) != APR_SUCCESS) {
		throw PEX("Unable to close outfile");
	}
	parser.wait();
	if (apr_file_close(infile) != APR_SUCCESS) {
		throw PEX("Unable to close infile");
	}
	return parser.stat();
}

// Main Thread function for fetching diffstats from a job queue
void SvnDiffstatThread::run()
{
	apr_pool_t *pool = svn_pool_create(d->pool);
	std::string revision;
	while (m_queue->getArg(&revision)) {
		apr_pool_t *subpool = svn_pool_create(pool);

		std::vector<std::string> revs = str::split(revision, ":");
		svn_revnum_t r1, r2;
		if (revs.size() > 1) {
			if (!str::stoi(revs[0], &r1) || !str::stoi(revs[1], &r2)) {
				goto parse_error;
			}
		} else {
			if (!str::stoi(revs[0], &r2)) {
				goto parse_error;
			}
			r1 = r2 - 1;
		}

		try {
			DiffstatPtr stat = diffstat(d, r1, r2, subpool);
			m_queue->done(revision, stat);
		} catch (const PepperException &ex) {
			Logger::err() << "Error: " << ex.where() << ": " << ex.what() << endl;
			m_queue->failed(revision);
		}
		goto next;

parse_error:
		PEX(std::string("Error parsing revision number ") + revision);
		m_queue->failed(revision);
next:
		svn_pool_destroy(subpool);
	}
	svn_pool_destroy(pool);
}
