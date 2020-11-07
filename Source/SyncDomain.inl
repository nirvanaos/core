#ifndef NIRVANA_CORE_SYNCDOMAIN_INL_
#define NIRVANA_CORE_SYNCDOMAIN_INL_

#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

inline
void SyncDomain::schedule (ExecDomain& ed)
{
	verify (queue_.insert (ed.deadline (), &ed));
	schedule ();
}

}
}

#endif
