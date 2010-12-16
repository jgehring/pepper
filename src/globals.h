/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: globals.h
 * Global variables (prototypes)
 */


#ifndef GLOBALS_H_
#define GLOBALS_H_


#include "syslib/parallel.h"


namespace Globals
{

extern sys::parallel::Mutex cacheMutex;

}


#endif // GLOBALS_H_
