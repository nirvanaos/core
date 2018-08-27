// Nirvana project.
// Protection domain.
#ifndef NIRVANA_CORE_PROTDOMAIN_H_
#define NIRVANA_CORE_PROTDOMAIN_H_

#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

class ProtDomain
{

private:
	PriorityQueueT <SyncDomain> queue_;
};

}
}

#endif
