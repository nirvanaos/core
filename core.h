// Nirvana project.
// Core implementation definitions

#ifndef NIRVANA_CORE_CORE_H_
#define NIRVANA_CORE_CORE_H_

#include <Nirvana.h>
#include <assert.h>
#include "config.h"
#ifdef _WIN32
#include "Windows/MemoryWindows.h"
#endif

namespace Nirvana {

#ifdef _WIN32

inline Memory_ptr prot_domain_memory ()
{
	return Windows::MemoryWindows::_this ();
}

inline void initialize ()
{
	Windows::MemoryWindows::initialize ();
}

inline void terminate ()
{
	Windows::MemoryWindows::terminate ();
}

#else
#error Unknown platform.
#endif

}

#endif
