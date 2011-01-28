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

	PyRun_SimpleString(utils::strprintf("\
import sys \n\
from cStringIO import StringIO \n\
from mercurial import ui,hg,commands \n\
myui = ui.ui() \n\
myui.quiet = True \n\
repo = hg.repository(myui, '%s') \n\n", repo.c_str()).c_str());
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
	PyRun_SimpleString("myui.quiet = False\n");
	std::string out = hgcmd("tags");
	PyRun_SimpleString("myui.quiet = True\n");

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

// Returns a revision iterator for the given branch
Backend::LogIterator *MercurialBackend::iterator(const std::string &branch)
{
	// Request log from HEAD to 0, so follow_first is effective
	std::string out = hgcmd("log", utils::strprintf("date=None, user=None, follow_first=True, quiet=None, rev=[\"%s:0\"]", (head(branch)).c_str()));
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
/*
	std::cout << utils::strprintf("\
sys.stdout = stdout = StringIO()\n\
res = commands.%s(myui, repo, %s)\n", cmd.c_str(), args.c_str()).c_str() << std::endl;
*/
	PyRun_SimpleString(utils::strprintf("\
sys.stdout = stdout = StringIO()\n\
res = commands.%s(myui, repo, %s)\n", cmd.c_str(), args.c_str()).c_str());
	PyObject *object = PyObject_GetAttrString(pModule, "stdout");
	char *method = strdup("getvalue");
	PyObject *output = PyObject_CallMethod(object, method, NULL);
	free(method);
	return std::string(PyString_AsString(output));
}
