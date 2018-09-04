#ifndef NIRVANA_CORE_THREADBACKGROUND_H_
#define NIRVANA_CORE_THREADBACKGROUND_H_

#include "Thread.h"

#ifdef _WIN32
#include "Windows/ThreadBackgroundBase.h"
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
};

}
}

#endif
