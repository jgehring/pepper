/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2011 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: report.h
 * Report script access and invocation (interface)
 */


#ifndef REPORT_H_
#define REPORT_H_


#include <iostream>

class Backend;


namespace Report
{

int run(const std::string &script, Backend *backend);

void printHelp(const std::string &script);

void listReports(std::ostream &out = std::cout);

} // namespace report


#endif // REPORT_H_
