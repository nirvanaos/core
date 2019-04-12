#ifndef NIRVANA_CORE_THREADBACKGROUND_H_
#define NIRVANA_CORE_THREADBACKGROUND_H_

#include "Thread.h"
#include "Port/ThreadBackground.h"

namespace Nirvana {
namespace Core {

class ThreadBackground :
	public Port::ThreadBackground
{
public:
	/// Temporary boost the priority for time-critical operations.
	/// \param boost `true` - raise priority above worker thread, `false` - down priority to background.
	virtual void boost_priority (bool boost);
};

}
}

#endif
