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

// Forks the process, running the given command and returning a file descriptor for reading
static int forkread(const char *cmd, const char * const *argv, int *pid = NULL)
{
	int fds[2];
	if (pipe(fds) == -1) {
		throw PEX_ERRNO();
	}

	// NOTE: fork() is slow if the application allocated a large
	// amount of memory, but all child calls after vfork() other than
	// exec*() result in undefined behavior.
	int cpid = fork();
	if (cpid < 0) {
		throw PEX_ERRNO();
	} else if (cpid == 0) {
		// Child process: close read end
		close(fds[0]);
		if (dup2(fds[1], STDOUT_FILENO) == -1) {
			perror("Error duplicating pipe");
			_exit(255);
		}
		close(fds[1]);
		::execv(cmd, (char * const *)argv);
		// NOTE: Needs to be changed to _exit() if vfork() is being used above
		exit(127);
	}

	// Parent process: close write end
	close(fds[1]);

	if (pid != NULL) {
		*pid = cpid;
	}
	return fds[0];
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
	int fd = forkread(cmd, argv, &pid);

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

	close(fd);

	// Wait for child
	waitpid(pid, ret, 0);
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
	int fd;
	int pid;
	const char *argv[9];
};

// Constructor
PopenStreambuf::PopenStreambuf(const char *cmd, const char *arg1, const char *arg2, const char *arg3, const char *arg4, const char *arg5, const char *arg6, const char *arg7)
	: std::streambuf(), d(new PopenStreambufData()), m_putback(8), m_buffer(4096 + 8)
{
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

	d->fd = forkread(cmd, d->argv, &d->pid);
}

// Destructor
PopenStreambuf::~PopenStreambuf()
{
	if (d->fd >= 0) {
		close();
	}
	delete d;
}

// Closes the process and returns its exit code
int PopenStreambuf::close()
{
	if (::close(d->fd) == -1)  {
		throw PEX_ERRNO();
	}
	d->fd = -1;

	int status;
	if (waitpid(d->pid, &status, 0) == -1) {
		throw PEX_ERRNO();
	}
	return status;
}

// Actual data fetching from pipe
PopenStreambuf::int_type PopenStreambuf::underflow()
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
	n = read(d->fd, start, n);
	if (n <= 0) { // TODO: Error if n < 0
		return traits_type::eof();
	}

	// Set buffer pointers
	setg(base, start, start + n);
	return traits_type::to_int_type(*gptr());
}

} // namespace io

} // namespace sys
