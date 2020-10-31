#ifndef NIRVANA_CORE_SYNCHRONIZED_H_
#define NIRVANA_CORE_SYNCHRONIZED_H_

#include "SyncContext.h"

namespace Nirvana {
namespace Core {

class Synchronized
{
public:
	Synchronized () :
		call_context_ (&SyncContext::current ())
	{}

	Synchronized (SyncContext& target) :
		call_context_ (&SyncContext::current ())
	{
		target.enter (false);
	}

	~Synchronized ()
	{
		call_context_->enter (true);
	}

	SyncContext& call_context () const
	{
		return *call_context_;
	}

private:
	Core_var <SyncContext> call_context_;
};

}
}

#endif
