// Nirvana project.
// Execution context (coroutine, fiber).

#ifndef NIRVANA_CORE_EXECCONTEXT_H_
#define NIRVANA_CORE_EXECCONTEXT_H_

#include "core.h"
#include <Nirvana/Runnable_c.h>
#include <Port/ExecContext.h>

namespace Nirvana {
namespace Core {

class ExecContext :
	public CoreObject,
	private Port::ExecContext
{
public:
	Port::ExecContext& port ()
	{
		return *this;
	}

	static ExecContext* current ();

	ExecContext () :
		Port::ExecContext ()
	{}

	template <class P>
	ExecContext (P param) :
		Port::ExecContext (param)
	{}

	::CORBA::Nirvana::Environment& environment ()
	{
		return environment_;
	}

	//! Switch to this context.
	void switch_to ();

	static void neutral_context_loop ();

protected:
	bool run ();

protected:
	friend void run_in_neutral_context (Runnable_ptr runnable);

	Runnable_var runnable_;
	::CORBA::Nirvana::Environment environment_;
};

}
}

#endif
