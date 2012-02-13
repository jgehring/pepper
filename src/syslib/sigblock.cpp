/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010-2012 Jonas Gehring
 *
 * Released under the GNU General Public License, version 3.
 * Please see the COPYING file in the source distribution for license
 * terms and conditions, or see http://www.gnu.org/licenses/.
 *
 * file: syslib/sigblock.cpp
 * Signal blocking utilities
 */


#include "main.h"

#include "sigblock.h"


namespace sys
{

namespace sigblock
{

volatile sig_atomic_t deferSignal = 0;
volatile sig_atomic_t signalPending = 0;
Handler *handler = NULL;

// Local signal handler
static void sighandler(int signum)
{
	if (deferSignal) {
		signalPending = signum;
		return;
	}

	// Call the installed handler
	if (handler) {
		(*handler)(signum);
	}

	signal(signum, SIG_DFL);
	raise(signum);
}

// Installs a signal handler for the given signals
void block(int n, int *signums, Handler *handler)
{
	sys::sigblock::handler = handler;
	for (int i = 0; i < n; i++) {
		signal(signums[i], sys::sigblock::sighandler);
	}
}


// Constructor
Deferrer::Deferrer()
{
	++deferSignal;
}

// Destructor
Deferrer::~Deferrer()
{
	// Handle signals?
	--deferSignal;
	if (deferSignal == 0 && signalPending != 0) {
		raise(signalPending);
	}
}

} // namespace sigblock

} // namespace sys
