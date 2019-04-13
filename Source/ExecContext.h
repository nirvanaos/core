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
	public Port::ExecContext
{
public:
	static ExecContext* current ();

	ExecContext () :
		Port::ExecContext ()
	{}

	template <class P>
	ExecContext (P param) :
		Port::ExecContext (param)
	{}

	::CORBA::Environment& environment ()
	{
		return environment_;
	}

	/// Switch to this context.
	void switch_to ();

	static void run_in_neutral_context (Runnable_ptr runnable);

	static void neutral_context_loop ();

protected:
	bool run ();

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

	Runnable_var runnable_;
	::CORBA::Environment environment_;
};

}
}

#endif
