#ifndef NIRVANA_CORE_EXECUTOR_H_
#define NIRVANA_CORE_EXECUTOR_H_

#include "core.h"

namespace Nirvana {
namespace Core {

class Executor
{
public:
	virtual void execute (DeadlineTime deadline) = 0;
};

}
}

#endif

