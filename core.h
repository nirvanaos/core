// Nirvana core implementation definitions

#ifndef NIRVANA_CORE_H_
#define NIRVANA_CORE_H_

#include <ORB.h>
#include <cpplib.h>
#include <assert.h>
#include "config.h"

#undef verify

#ifdef NDEBUG

#define verify(exp) (exp)

#else

#define verify(exp) assert(exp)

#endif

#endif
