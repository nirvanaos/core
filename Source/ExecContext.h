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

	/// Switch to this context.
	void switch_to ();

	static void neutral_context_loop ();

	void run_in_context (Runnable& runnable) NIRVANA_NOEXCEPT
	{
		assert (this != &current ());
		runnable_ = &runnable;
		switch_to ();
	}

protected:
	void run ();

	void on_crash ();

protected:
	Core_var <Runnable> runnable_;
};

}
}

#endif
