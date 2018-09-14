#ifndef NIRVANA_CORE_THREAD_H_
#define NIRVANA_CORE_THREAD_H_

#include "core.h"
#include "ExecDomain.h"
#include "RandomGen.h"

#ifdef _WIN32
#include "Windows/ThreadWindows.h"
namespace Nirvana {
namespace Core {
typedef Windows::ThreadWindows ThreadBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class ExecDomain;

class Thread :
	public CoreObject,
	protected ThreadBase
{
public:
	/// Returns current thread.
	static Thread* current ()
	{
		return static_cast <Thread*> (ThreadBase::current ());
	}

	Thread () :
		ThreadBase (),
		rndgen_ ()
	{}

	template <class P>
	Thread (P param) :
		ThreadBase (param),
		rndgen_ ()
	{}

	/// Random number generator's accessor.
	RandomGen& rndgen ()
	{
		return rndgen_;
	}

	virtual ExecContext* neutral_context ()
	{
		assert (false);
		return nullptr;
	}

	void join () const
	{
		ThreadBase::join ();
	}

protected:
	/// Random number generator.
	RandomGen rndgen_;

	/// Pointer to the current execution domain.
	ExecDomain* exec_domain_;
};

}
}

#endif
