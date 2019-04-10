#ifndef NIRVANA_CORE_THREADBACKGROUND_H_
#define NIRVANA_CORE_THREADBACKGROUND_H_

#include "Thread.h"

#ifdef _WIN32
#include "PortThreadBackground.h"
namespace Nirvana {
namespace Core {
typedef Windows::ThreadBackgroundBase ThreadBackgroundBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class ThreadBackground :
	public ThreadBackgroundBase
{
public:
	/// Temporary boost the priority for time-critical operations.
	/// \param boost `true` - raise priority above worker thread, `false` - down priority to background.
	virtual void boost_priority (bool boost);
};

}
}

#endif
