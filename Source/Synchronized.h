#ifndef NIRVANA_CORE_SYNCHRONIZED_H_
#define NIRVANA_CORE_SYNCHRONIZED_H_

#include "SynchronizationContext.h"

namespace Nirvana {
namespace Core {

class Synchronized
{
public:
	Synchronized () :
		call_context_ (SynchronizationContext::current ())
	{}

	Synchronized (SynchronizationContext* target) :
		call_context_ (SynchronizationContext::current ())
	{
		target->enter (false);
	}

	~Synchronized ()
	{
		call_context_->enter (true);
	}

	SynchronizationContext* call_context () const
	{
		return call_context_;
	}

private:
	Core_var <SynchronizationContext> call_context_;
};

}
}

#endif
