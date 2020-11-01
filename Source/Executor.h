#ifndef NIRVANA_CORE_EXECUTOR_H_
#define NIRVANA_CORE_EXECUTOR_H_

#include "CoreInterface.h"

namespace Nirvana {
namespace Core {

class Executor : public CoreInterface
{
public:
	virtual void execute (DeadlineTime deadline, Word scheduler_error) = 0;
};

}
}

#endif

