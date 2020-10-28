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

	::CORBA::Nirvana::I_ptr <::CORBA::Nirvana::EnvironmentBridge> environment () const
	{
		return environment_;
	}

	/// Switch to this context.
	void switch_to ();

	static void neutral_context_loop ();

	void run_in_context (Runnable& runnable, CORBA::Nirvana::Interface_ptr environment)
	{
		assert (this != current ());
		runnable_ = &runnable;
		environment_ = environment;
		switch_to ();
	}

protected:
	bool run ();

	void on_crash ();

protected:
	Core_var <Runnable> runnable_;
	CORBA::Nirvana::I_var <CORBA::Nirvana::EnvironmentBridge> environment_;
};

}
}

#endif
