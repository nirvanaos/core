#ifndef NIRVANA_CORE_INITTERM_H_
#define NIRVANA_CORE_INITTERM_H_

#include "Binder.h"

namespace Nirvana {
namespace Core {

//! Called by Startup class from free sync domain after kernel initialization.
inline void initialize ()
{
	Binder::initialize ();
}

//! Called before the kernel termination.
inline void terminate ()
{
	Binder::terminate ();
}

}
}

#endif
