/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: syslib/fs.cpp
 * I/O classes and functions
 */


#include <cstring>

#include "main.h"

#ifdef USE_POPEN_NOSHELL
 #include "popen-noshell/popen_noshell.h"
#endif

#include "io.h"


namespace sys
{

namespace io
{

// Runs the specified command line and returns the output
std::string execv(int *ret, const char * const *argv)
{
#ifdef USE_POPEN_NOSHELL
	struct popen_noshell_pass_to_pclose pclose_arg;
	FILE *pipe = popen_noshell(argv[0], argv, "r", &pclose_arg, 0);
#else
	// Concatenate arguments, put possible meta characters in quotes
	std::string cmd;
	const char * const *ptr = argv;
	const char *metachars = "!\\$`\n|&;()<>";
	while (*ptr) {
		bool quote = false;
		for (unsigned int i = 0; i < strlen(metachars); i++) {
			if (strchr(*ptr, metachars[i]) != NULL) {
				quote = true;
				break;
			}
		}

		if (quote) {
			cmd += "\"";
			cmd += *ptr;
			cmd += "\"";
		} else {
			cmd += *ptr;
		}
		cmd += " ";
		++ptr;
	}
	FILE *pipe = popen(cmd.c_str(), "r");
#endif
	if (!pipe) {
		throw PEX(std::string("Unable to open pipe for command ")+argv[0]);
	}

	char buffer[128];
	std::string result;
	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe) != NULL) {
			result += buffer;
		}
	}

#ifdef USE_POPEN_NOSHELL
	int r = pclose_noshell(&pclose_arg);
#else
	int r = pclose(pipe);
#endif
	if (ret != NULL) {
		*ret = r;
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
	std::string out = execv(ret, argv);
	delete[] argv;
	return out;
}


// Internal data for PopenStreambuf
class PopenStreambufData
{
public:
	FILE *pipe;
	const char *argv[9];
#ifdef USE_POPEN_NOSHELL
	struct popen_noshell_pass_to_pclose pclose_arg;
#endif
};

// Constructor
PopenStreambuf::PopenStreambuf(const char *cmd, const char *arg1, const char *arg2, const char *arg3, const char *arg4, const char *arg5, const char *arg6, const char *arg7)
	: std::streambuf(), d(new PopenStreambufData()), m_putback(8), m_buffer(4096 + 8)
{
	d->argv[0] = cmd;
	d->argv[1] = arg1; d->argv[2] = arg2; d->argv[3] = arg3;
	d->argv[4] = arg4; d->argv[5] = arg5; d->argv[6] = arg6;
	d->argv[7] = arg7; d->argv[8] = NULL;

#ifdef USE_POPEN_NOSHELL
	d->pipe = popen_noshell(argv[0], argv, "r", &(d->pclose_arg), 0);
#else
	// Concatenate arguments, put possible meta characters in quotes
	std::string concat;
	const char * const *ptr = d->argv;
	const char *metachars = "!\\$`\n|&;()<>";
	while (*ptr) {
		bool quote = false;
		for (unsigned int i = 0; i < strlen(metachars); i++) {
			if (strchr(*ptr, metachars[i]) != NULL) {
				quote = true;
				break;
			}
		}

		if (quote) {
			concat += "\"";
			concat += *ptr;
			concat += "\"";
		} else {
			concat += *ptr;
		}
		concat += " ";
		++ptr;
	}
	d->pipe = popen(concat.c_str(), "r");
#endif

	if (!d->pipe) {
		throw PEX(std::string("Unable to open pipe for command ")+concat);
	}
}

// Destructor
PopenStreambuf::~PopenStreambuf()
{
	delete d;
}

// Closes the process and returns its exit code
int PopenStreambuf::close()
{
#ifdef USE_POPEN_NOSHELL
	return pclose_noshell(&(d->pclose_arg));
#else
	return pclose(d->pipe);
#endif
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

	if (feof(d->pipe)) {
		return traits_type::eof();
	}

	// start is now the start of the buffer, proper.
	// Read from fptr_ in to the provided buffer
	size_t n = m_buffer.size() - (start - base);
	n = fread(start, 1, n, d->pipe);
	if (n == 0) {
		return traits_type::eof();
	}

	// Set buffer pointers
	setg(base, start, start + n);
	return traits_type::to_int_type(*gptr());
}

} // namespace io

} // namespace sys
