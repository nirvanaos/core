#include "Event.h"

namespace Nirvana {
namespace Core {

void Event::wait () {
	if (!signalled_) {
		ExecDomain::current ().suspend_prepare ();
		ExecContext::run_in_neutral_context (wait_op_);
	}
}

void Event::WaitOp::run ()
{
	ExecDomain& ed = ExecDomain::current ();
	ed.suspend_prepared ();
	obj_.push (ed);
	if (obj_.signalled_) {
		ExecDomain* ed = obj_.pop ();
		if (ed)
			ed->resume ();
	}
}

void Event::signal () NIRVANA_NOEXCEPT
{
	// TODO: Can cause priority inversion.
	// We should sort released ED by deadline.
	assert (!signalled_);
	signalled_ = true;
	while (ExecDomain* ed = pop ()) {
		ed->resume ();
	}
}

}
}
