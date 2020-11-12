#ifndef NIRVANA_CORE_WAITLIST_H_
#define NIRVANA_CORE_WAITLIST_H_

#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

class WaitList :
	public CoreObject
{
public:
	WaitList ();
	~WaitList ();

private:
	ExecDomain& worker_;
	ExecDomain* wait_list_;
};

}
}

#endif
