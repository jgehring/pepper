/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: syslib/sigblock.h
 * Signal blocking utilities (interface)
 */


#ifndef SYS_SIGBLOCK_H_
#define SYS_SIGBLOCK_H_


#include <signal.h>


namespace sys
{

namespace sigblock
{

struct Handler
{
	virtual void operator()(int signum) = 0;
};

void block(int n, int *signumn, Handler *handler);

// This class guard sensitive code segments with its life
class Deferrer
{
	public:
		Deferrer();
		~Deferrer();

	private:
		Deferrer(const Deferrer &);
		Deferrer &operator=(const Deferrer &);
};

} // namespace sigblock

} // namespace sys

#define SIGBLOCK_DEFER() do { sys::sigblock::Deferrer defer__; } while (0)


#endif // SYS_SIGBLOCK_H_
