/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-present Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: gnuplot.cpp
 * Bidirectional Gnuplot pipe
 */


#include "gnuplot.h"

#include "syslib/fs.h"
#include "syslib/parallel.h"

using sys::io::PopenStreambuf;


// Internal reader thread
class StreambufReader : public sys::parallel::Thread
{
public:
	StreambufReader(PopenStreambuf *buf, std::ostream &out)
		: m_buf(buf), m_out(out)
	{
	}

protected:
	void run()
	{
		char buffer[1024];
		std::istream in(m_buf);
		while (in.good()) {
			in.read(buffer, sizeof(buffer));
			m_out.write(buffer, in.gcount());
		}
	}

private:
	PopenStreambuf *m_buf;
	std::ostream &m_out;
};


// Standard output terminal
std::string Gnuplot::s_stdTerminal;

// Constructor
Gnuplot::Gnuplot(const char * const *args, std::ostream &out)
{
	std::string path = sys::fs::which("gnuplot");

	m_buf = new PopenStreambuf(path.c_str(), args, std::ios::in | std::ios::out);
	m_reader = new StreambufReader(m_buf, out);
	m_reader->start();
	m_pipe = new std::ostream(m_buf);
}

// Destructor
Gnuplot::~Gnuplot()
{
	delete m_pipe;
	m_buf->closeWrite();
	m_reader->wait();
	delete m_reader;
	delete m_buf;
}

// Writes a command to the Gnuplot pipe
void Gnuplot::cmd(const std::string &str)
{
	*m_pipe << str << "\n" << std::flush;
}

// Writes a command to the Gnuplot pipe
void Gnuplot::cmd(const char *str, size_t len)
{
	m_pipe->write(str, len);
	*m_pipe << std::flush;
}
