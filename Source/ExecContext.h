// Nirvana project.
// Execution context (coroutine, fiber).

#ifndef NIRVANA_CORE_EXECCONTEXT_H_
#define NIRVANA_CORE_EXECCONTEXT_H_

#include "core.h"
#include "Runnable.h"
#include <Port/ExecContext.h>

namespace Nirvana {
namespace Core {

class ExecContext :
	private Port::ExecContext
{
public:
	Port::ExecContext& port ()
	{
		return *this;
	}

	static ExecContext* current ();

	template <class ... Args>
	ExecContext (Args ... args) :
		Port::ExecContext (std::forward <Args> (args)...)
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
	friend void run_in_neutral_context (Runnable& runnable);

	Core_var <Runnable> runnable_;
	::CORBA::Nirvana::Environment environment_;
};

}
}

#endif
