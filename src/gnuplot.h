/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: gnuplot.h
 * Bidirectional Gnuplot pipe (interface)
 */


#ifndef _GNUPLOT_H
#define _GNUPLOT_H


#include <iostream>
#include <string>
#include <vector>

#include "syslib/io.h"

class StreambufReader;


class Gnuplot
{
	public:
		Gnuplot(const char * const *args, std::ostream &out = std::cout);
		~Gnuplot();

		void cmd(const std::string &str);

		inline Gnuplot& operator<<(const std::string &str) {
			cmd(str);
			return (*this);
		}

	private:
		sys::io::PopenStreambuf *m_buf;
		StreambufReader *m_reader;
		std::ostream *m_pipe;
		static std::string s_stdTerminal;
};


#endif // _GNUPLOT_H
