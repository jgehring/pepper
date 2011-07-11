/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: mercurial.cpp
 * Mercurial repository backend
 */


#include "Python.h"
#include "main.h" // Avoid compilation warnings

#include <algorithm>

#include "logger.h"
#include "options.h"
#include "revision.h"
#include "utils.h"

#include "syslib/fs.h"

#include "backends/mercurial.h"


// Constructor
MercurialBackend::MercurialBackend(const Options &options)
	: Backend(options)
{
	Py_Initialize();
}

// Destructor
MercurialBackend::~MercurialBackend()
{
	Py_Finalize();
}

// Initializes the backend
void MercurialBackend::init()
{
	std::string repo = m_opts.repository();
	if (!sys::fs::dirExists(repo + "/.hg")) {
		throw PEX(utils::strprintf("Not a mercurial repository: %s", repo.c_str()));
	}

	int res = simpleString(utils::strprintf("\
import sys \n\
from cStringIO import StringIO\n\
from mercurial import ui,hg,commands \n\
sys.stderr = stderr = StringIO()\n\
myui = ui.ui() \n\
myui.quiet = True \n\
repo = hg.repository(myui, '%s') \n\n", repo.c_str()));
	assert(res == 0);
}

// Returns true if this backend is able to access the given repository
bool MercurialBackend::handles(const std::string &url)
{
	return sys::fs::dirExists(url+"/.hg");
}

// Returns a unique identifier for this repository
std::string MercurialBackend::uuid()
{
	// Use the ID of the first commit of the master branch as the UUID.
	std::string out = hgcmd("log", "date=None, user=None, quiet=None, rev=\"0\"");
	size_t pos = out.find(':');
	if (pos != std::string::npos) {
		out = out.substr(pos+1);
	}
	return utils::trim(out);
}

// Returns the HEAD revision for the current branch
std::string MercurialBackend::head(const std::string &branch)
{
	std::string out = hgcmd("log", utils::strprintf("date=None, rev=None, user=None, quiet=None, limit=1, branch=[\"%s\"]", branch.c_str()));
	size_t pos = out.find(':');
	if (pos != std::string::npos) {
		out = out.substr(pos+1);
	}
	return utils::trim(out);
}

// Returns the currently checked out branch
std::string MercurialBackend::mainBranch()
{
	std::string out = hgcmd("branch");
	return utils::trim(out);
}

// Returns a list of available branches
std::vector<std::string> MercurialBackend::branches()
{
#if 1
	std::string out = hgcmd("branches");
#else
	int ret;
	std::string out = sys::io::exec(&ret, "hg", "--noninteractive", "--repository", m_opts.repository().c_str(), "branches", "--quiet");
	if (ret != 0) {
		throw PEX(utils::strprintf("Unable to retreive the list of branches (%d)", ret));
	}
#endif
	std::vector<std::string> branches = utils::split(out, "\n");
	while (!branches.empty() && branches[branches.size()-1].empty()) {
		branches.pop_back();
	}
	return branches;
}

// Returns a list of available tags
std::vector<Tag> MercurialBackend::tags()
{
	// TODO: Determine correct keyword for non-quiet output, if any
	simpleString("myui.quiet = False\n");
	std::string out = hgcmd("tags");
	simpleString("myui.quiet = True\n");

	std::vector<std::string> lines = utils::split(out, "\n");
	std::vector<Tag> tags;
	for (unsigned int i = 0; i < lines.size(); i++) {
		size_t pos = lines[i].find(" ");
		if ((pos = lines[i].find(" ")) == std::string::npos) {
			continue;
		}
		std::string name = lines[i].substr(0, pos);
		if ((pos = lines[i].find(":", pos)) == std::string::npos) {
			continue;
		}
		std::string id = lines[i].substr(pos+1);
		tags.push_back(Tag(id, name));
	}

	return tags;
}

// Returns a diffstat for the specified revision
Diffstat MercurialBackend::diffstat(const std::string &id)
{
	std::vector<std::string> ids = utils::split(id, ":");
#if 1
	std::string out;
	if (ids.size() > 1) {
		out = hgcmd("diff", utils::strprintf("rev=[\"%s:%s\"]", ids[0].c_str(), ids[1].c_str()));
	} else {
		out = hgcmd("diff", utils::strprintf("change=\"%s\"", ids[0].c_str()));
	}
#else
	std::string out = sys::io::exec(hgcmd()+" diff --change "+id);
#endif
	std::istringstream in(out);
	return DiffParser::parse(in);
}

// Returns a file listing for the given revision (defaults to HEAD)
std::vector<std::string> MercurialBackend::tree(const std::string &id)
{
	std::string out = hgcmd("status", utils::strprintf("change=\"%s\", all=True, no_status=True", id.c_str()));
	std::vector<std::string> contents = utils::split(out, "\n");
	if (!contents.empty() && contents[contents.size()-1].empty()) {
		contents.pop_back();
	}
	return contents;
}

// Returns the file contents of the given path at the given revision (defaults to HEAD)
std::string MercurialBackend::cat(const std::string &path, const std::string &id)
{
	// stdout redirection to a cStringIO object doesn't work here,
	// because Mercurial's cat will close the file after writing,
	// which discards all contents of a cStringIO object.
	std::string filename;
	FILE *f = sys::fs::mkstemp(&filename);
	hgcmd("cat", utils::strprintf("\"%s\", output=\"%s\", rev=\"%s\"", (m_opts.repository() + "/" + path).c_str(), filename.c_str(), id.c_str()));

	char buf[4096];
	size_t size;
	std::string out;
	while ((size = fread(buf, 1, sizeof(buf), f)) != 0) {
		out += std::string(buf, size);
	}
	if (ferror(f)) {
		throw PEX("Error reading stream");
	}
	fclose(f);
	sys::fs::unlink(filename);

	return out;
}

// Returns a revision iterator for the given branch
Backend::LogIterator *MercurialBackend::iterator(const std::string &branch, int64_t start, int64_t end)
{
	std::string date = "None";
	if (start >= 0) {
		if (end >= 0) {
			date = utils::strprintf("\"%lld 0 to %lld 0\"", start, end);
		} else {
			date = utils::strprintf("\">%lld 0\"", start);
		}
	} else if (end >= 0) {
		date = utils::strprintf("\"<%lld 0\"", end);
	}

	// Request log from HEAD to 0, so follow_first is effective
	std::string out = hgcmd("log", utils::strprintf("date=%s, user=None, follow_first=True, quiet=None, rev=[\"%s:0\"]", date.c_str(), (head(branch)).c_str()));
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

	// Add parent revisions, so diffstat fetching will give correct results
	for (int i = revisions.size()-1; i > 0; i--) {
		revisions[i] = revisions[i-1] + ":" + revisions[i];
	}

	return new LogIterator(revisions);
}

// Returns the revision data for the given ID
Revision *MercurialBackend::revision(const std::string &id)
{
	std::vector<std::string> ids = utils::split(id, ":");
#if 1
	std::string meta = hgcmd("log", utils::strprintf("rev=[\"%s\"], date=None, user=None, template=\"{date|hgdate}\\n{author|person}\\n{desc}\"", ids.back().c_str()));
#else
	std::string meta = sys::io::exec(hgcmd()+" log -r "+id+" --template=\"{date|hgdate}\n{author|person}\n{desc}\"");
#endif
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
	return std::string("hg --noninteractive --repository ") + m_opts.repository();
}

// Returns the Python code for a hg command
std::string MercurialBackend::hgcmd(const std::string &cmd, const std::string &args) const
{
	PyObject *pModule = PyImport_AddModule("__main__"); 
	simpleString(utils::strprintf("\
stdout = \"\"\n\
stderr.truncate(0)\n\
res = -127\n\
try:\n\
	myui.pushbuffer()\n\
	res = commands.%s(myui, repo, %s)\n\
	stdout = myui.popbuffer()\n\
except:\n\
	res = -127\n\
	stderr.write(sys.exc_info()[0])\n\
", cmd.c_str(), args.c_str()));

	// Check return value
	PyObject *object = PyObject_GetAttrString(pModule, "res");
	Py_ssize_t res = (object == Py_None ? 0 : PyNumber_AsSsize_t(object, NULL));

	if (res != 0) {
		PDEBUG << "res = " << res << std::endl;
		// Throw exception
		object = PyObject_GetAttrString(pModule, "stderr");
		PyObject *output = PyObject_CallMethod(object, "getvalue", NULL);
		assert(output != NULL);
		throw PEX(utils::trim(PyString_AsString(output)));
	}

	// Return stdout
	object = PyObject_GetAttrString(pModule, "stdout");
	assert(object != NULL);
	return std::string(PyString_AsString(object));
}

// Wrapper for PyRun_SimpleString()
int MercurialBackend::simpleString(const std::string &str) const
{
	PDEBUG << str;
	return PyRun_SimpleString(str.c_str());
}
