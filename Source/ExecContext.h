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
	Port::ExecContext& port () NIRVANA_NOEXCEPT
	{
		return *this;
	}

	static ExecContext& current () NIRVANA_NOEXCEPT;

	template <class ... Args>
	ExecContext (Args ... args) :
		Port::ExecContext (std::forward <Args> (args)...)
	{}

	/// Switch to this context.
	void switch_to () NIRVANA_NOEXCEPT;

	static void neutral_context_loop () NIRVANA_NOEXCEPT;

	void run_in_context (Runnable& runnable) NIRVANA_NOEXCEPT
	{
		assert (this != &current ());
		runnable_ = &runnable;
		switch_to ();
	}

protected:
	void run () NIRVANA_NOEXCEPT;

	void on_crash (CORBA::SystemException::Code err) NIRVANA_NOEXCEPT;

protected:
	Core_var <Runnable> runnable_;
};

}
}

#endif
