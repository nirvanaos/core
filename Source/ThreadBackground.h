#ifndef NIRVANA_CORE_THREADBACKGROUND_H_
#define NIRVANA_CORE_THREADBACKGROUND_H_

#include "Thread.h"
#include "Port/ThreadBackground.h"

namespace Nirvana {
namespace Core {

class ThreadBackground :
	public Port::ThreadBackground,
	public SynchronizationContext
{
public:

};

}
}

#endif
