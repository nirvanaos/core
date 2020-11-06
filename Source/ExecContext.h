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
	friend class Port::ExecContext;
public:
	// Implementation - specific methods must be called explicitly.
	Port::ExecContext& port ()
	{
		return *this;
	}

	static ExecContext& current ();

	template <class ... Args>
	ExecContext (Args ... args) :
		Port::ExecContext (std::forward <Args> (args)...)
	{}

	::CORBA::Nirvana::Interface_ptr environment () const
	{
		return environment_;
	}

	/// Switch to this context.
	void switch_to ();

	static void neutral_context_loop ();

	void run_in_context (Runnable& runnable, CORBA::Nirvana::Interface_ptr environment)
	{
		assert (this != &current ());
		runnable_ = &runnable;
		environment_ = environment;
		switch_to ();
	}

	/// Abort current context due to a unrecoverable error.
	static void abort ()
	{
		Port::ExecContext::abort ();
	}

protected:
	bool run ();

	void on_crash (CORBA::Exception::Code code);

protected:
	Core_var <Runnable> runnable_;
	CORBA::Nirvana::Interface_var environment_;
};

}
}

#endif
