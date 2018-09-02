#ifndef NIRVANA_CORE_THREAD_H_
#define NIRVANA_CORE_THREAD_H_

#include "core.h"
#include "ExecDomain.h"

#ifdef _WIN32
#include "Windows/ThreadBase.h"
namespace Nirvana {
namespace Core {
typedef Windows::ThreadBase ThreadBase;
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
		rndgen_ ((unsigned)this)
	{}

	/// Random number generator's accessor.
	RandomGen& rndgen ()
	{
		return rndgen_;
	}

	typedef void (*NeutralProc) (void*);

	/// Call in neutral context.
	void call_in_neutral (NeutralProc proc, void* param)
	{
		neutral_proc_ = proc;
		neutral_proc_param_ = param;
		ThreadBase::switch_to_neutral ();
	}

	bool neutral_context_call ()
	{
		NeutralProc proc;
		if (proc = neutral_proc_) {
			neutral_proc_ = nullptr;
			(proc) (neutral_proc_param_);
			if (exec_domain_) {
				exec_domain_->switch_to ();
				return true;
			}
		}
		return false;
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

	/// Address of procedure to call in neutral context.
	NeutralProc neutral_proc_;

	/// Parameter for procedure called in neutral context.
	void* neutral_proc_param_;
};

}
}

#endif
