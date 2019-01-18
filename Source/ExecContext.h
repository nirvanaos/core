// Nirvana project.
// Execution context (coroutine, fiber).

#ifndef NIRVANA_CORE_EXECCONTEXT_H_
#define NIRVANA_CORE_EXECCONTEXT_H_

#include "core.h"
#include <Interface/Runnable.h>

#ifdef _WIN32
#include "Windows/ExecContextWindows.h"
namespace Nirvana {
namespace Core {
typedef Windows::ExecContextWindows ExecContextBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class ExecContext :
	public CoreObject,
	protected ExecContextBase
{
public:
	static ExecContext* current ()
	{
		return static_cast <ExecContext*> (ExecContextBase::current ());
	}

	ExecContext () :
		ExecContextBase ()
	{}

	template <class P>
	ExecContext (P param) :
		ExecContextBase (param)
	{}

	::CORBA::Environment& environment ()
	{
		return environment_;
	}

	/// Switch to this context.
	void switch_to ()
	{
		assert (current () != this);
		ExecContextBase::switch_to ();
	}

	static void run_in_neutral_context (Runnable_ptr runnable);

	static void neutral_context_loop ();

protected:
	void run ();

protected:
	class RunnableHolder :
		public Runnable_var
	{
	public:
		RunnableHolder& operator = (Runnable_ptr p)
		{
			Runnable_var::operator = (p);
			return *this;
		}

		void run (::CORBA::Environment& env);
	};

	RunnableHolder runnable_;
	::CORBA::Environment environment_;
};

}
}

#endif
