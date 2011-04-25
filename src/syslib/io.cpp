/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: syslib/io.cpp
 * I/O classes and functions
 */


#include "main.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/wait.h>

#include "logger.h"

#include "io.h"


namespace sys
{

namespace io
{

// Small struct used as the return type for forkrw()
struct Filedes {
	int r, w;

	Filedes(int r = -1, int w = -1) : r(r), w(w) { }
};

static sys::parallel::Mutex forkMutex;

// Forks the process, running the given command and returning file
// descriptors for reading and writing
static Filedes forkrw(const char *cmd, const char * const *argv, int *pid = NULL, std::ios::open_mode mode = std::ios::in)
{
	sys::parallel::MutexLocker locker(&forkMutex);

	int rfds[2] = {-1, -1}, wfds[2] = {-1, -1};
	if ((mode & std::ios::in) && pipe(rfds) == -1) {
		throw PEX_ERRNO();
	}
	if ((mode & std::ios::out) && pipe(wfds) == -1) {
		throw PEX_ERRNO();
	}

	// NOTE: fork() is slow if the application allocated a large
	// amount of memory, but all child calls after vfork() other than
	// exec*() result in undefined behavior.
	int cpid = fork();
	if (cpid < 0) {
		throw PEX_ERRNO();
	} else if (cpid == 0) {
		// Child process: close unused pipe ends and redirect
		// the others
		if (mode & std::ios::in) {
			close(rfds[0]);
			if (dup2(rfds[1], STDOUT_FILENO) == -1) {
				perror("Error duplicating pipe");
				_exit(255);
			}
			close(rfds[1]);
		}
		if (mode & std::ios::out) {
			close(wfds[1]);
			if (dup2(wfds[0], STDIN_FILENO) == -1) {
				perror("Error duplicating pipe");
				_exit(255);
			}
			close(wfds[0]);
		}

		::execv(cmd, (char * const *)argv);
		perror("Error running program");

		// NOTE: Needs to be changed to _exit() if vfork() is being used above
		exit(127);
	}

	// Parent process: close unused pipe ends
	if ((mode & std::ios::in) && close(rfds[1]) == -1) {
		throw PEX_ERRNO();
	}
	if ((mode & std::ios::out) && close(wfds[0]) == -1) {
		throw PEX_ERRNO();
	}

	if (pid != NULL) {
		*pid = cpid;
	}
	return Filedes(rfds[0], wfds[1]);
}


// Checks whether the given file is a terminal
bool isterm(FILE *f)
{
	return (bool)isatty(fileno(f));
}

// Runs the specified command line and returns the output
std::string execv(int *ret, const char *cmd, const char * const *argv)
{
	int pid;
	int fd = forkrw(cmd, argv, &pid, std::ios::in).r;

	// Read child output from pipe
	char buf[512];
	std::string result;
	int n;
	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		result.append(buf, n);
	}
	if (n < 0) {
		throw PEX_ERRNO();
	}

	if (close(fd) == -1) {
		throw PEX_ERRNO();
	}

	// Wait for child process
	if (waitpid(pid, ret, 0) == -1) {
		throw PEX_ERRNO();
	}
	return result;
}

// Runs the specified command line and returns the output
std::string exec(int *ret, const char *cmd, const char *arg1, const char *arg2, const char *arg3, const char *arg4, const char *arg5, const char *arg6, const char *arg7)
{
	const char **argv = new const char *[9];
	argv[0] = cmd;
	argv[1] = arg1; argv[2] = arg2; argv[3] = arg3;
	argv[4] = arg4; argv[5] = arg5; argv[6] = arg6;
	argv[7] = arg7; argv[8] = NULL;
	PTRACE << cmd << " "
		<< (arg1 ? arg1 : "") << " "
		<< (arg2 ? arg2 : "") << " "
		<< (arg3 ? arg3 : "") << " "
		<< (arg4 ? arg4 : "") << " "
		<< (arg5 ? arg5 : "") << " "
		<< (arg6 ? arg6 : "") << " "
		<< (arg7 ? arg7 : "") << endl;
	std::string out = execv(ret, cmd, argv);
	delete[] argv;
	return out;
}


// Internal data for PopenStreambuf
class PopenStreambufData
{
public:
	Filedes pipe;
	int pid;
	const char *argv[9];
};

// Constructor
PopenStreambuf::PopenStreambuf(const char *cmd, const char *arg1, const char *arg2, const char *arg3, const char *arg4, const char *arg5, const char *arg6, const char *arg7, std::ios::open_mode m)
	: std::streambuf(), d(new PopenStreambufData()), m_mode(m), m_putback(8)
{
	if (m & std::ios::in) {
		m_inbuffer = std::vector<char>(4096 + 8);
		setg(0, 0, 0);
	}
	if (m & std::ios::out) {
		m_outbuffer = std::vector<char>(4096 + 8);
		setp(&m_outbuffer.front(), &m_outbuffer.front() + m_outbuffer.size() - 1);
	}

	d->argv[0] = cmd;
	d->argv[1] = arg1; d->argv[2] = arg2; d->argv[3] = arg3;
	d->argv[4] = arg4; d->argv[5] = arg5; d->argv[6] = arg6;
	d->argv[7] = arg7; d->argv[8] = NULL;

	PTRACE << cmd << " "
		<< (arg1 ? arg1 : "") << " "
		<< (arg2 ? arg2 : "") << " "
		<< (arg3 ? arg3 : "") << " "
		<< (arg4 ? arg4 : "") << " "
		<< (arg5 ? arg5 : "") << " "
		<< (arg6 ? arg6 : "") << " "
		<< (arg7 ? arg7 : "") << endl;

	d->pipe = forkrw(cmd, d->argv, &d->pid, m);
}

// Destructor
PopenStreambuf::~PopenStreambuf()
{
	if (d->pid >= 0) {
		close();
	}
	delete d;
}

// Closes the process and returns its exit code
int PopenStreambuf::close()
{
	if (d->pipe.r >= 0 && ::close(d->pipe.r) == -1) {
		throw PEX_ERRNO();
	}
	if (d->pipe.w >= 0 && ::close(d->pipe.w) == -1) {
		throw PEX_ERRNO();
	}
	d->pipe.r = d->pipe.w = -1;

	int status = -1;
	if (d->pid >= 0) {
		if (waitpid(d->pid, &status, 0) == -1) {
			throw PEX_ERRNO();
		}
	}
	d->pid = -1;

	return status;
}

// Actual data fetching from pipe
PopenStreambuf::int_type PopenStreambuf::underflow()
{
	if (gptr() < egptr()) { // buffer not exhausted
		return traits_type::to_int_type(*gptr());
	}
	if (!(m_mode & std::ios::in) || d->pipe.r < 0) {
		return traits_type::eof();
	}

	char *base = &m_inbuffer.front();
	char *start = base;

	if (eback() == base) { // true when this isn't the first fill
		// Make arrangements for putback characters
		memmove(base, egptr() - m_putback, m_putback);
		start += m_putback;
	}

	// start is now the start of the buffer, proper.
	// Read from fptr_ in to the provided buffer
	size_t n = m_inbuffer.size() - (start - base);
	n = read(d->pipe.r, start, n);
	if (n <= 0) { // TODO: Error if n < 0
		return traits_type::eof();
	}

	// Set buffer pointers
	setg(base, start, start + n);
	return traits_type::to_int_type(*gptr());
}

// Flush write buffer
PopenStreambuf::int_type PopenStreambuf::sync()
{
	if (overflow(traits_type::eof()) == traits_type::eof()) {
		return -1;
	}
	return 0;
}

// Actual data writing to pipe
PopenStreambuf::int_type PopenStreambuf::overflow(int_type c)
{
	if (!(m_mode & std::ios::out) || d->pipe.w < 0) {
		return traits_type::eof();
	}

	char *base = &m_outbuffer.front();
	char *end = pptr();

	// If c isn't EOF, write it, too 
	if (c != traits_type::eof()) {
		*end++ = traits_type::to_char_type(c);
	}

	// Write characters to pipe
	size_t n = end - base;
	n = write(d->pipe.w, base, n);
	if (n <= 0) { // TODO: Error if n < 0
		setp(0, 0);
		return traits_type::eof();
	}

	// Set buffer pointers
	setp(base, base + m_outbuffer.size() - 1);
	return traits_type::to_int_type(*pptr());
}

} // namespace io

} // namespace sys
