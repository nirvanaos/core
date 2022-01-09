#include "Event.h"

namespace Nirvana {
namespace Core {

void Event::WaitOp::run ()
{
	if (!obj_.signalled_) {
		ExecDomain* ed = Thread::current ().exec_domain ();
		assert (ed);
		ed->suspend ();
		obj_.push (*ed);
		if (obj_.signalled_) {
			ExecDomain* ed = obj_.pop ();
			if (ed)
				ed->resume ();
		}
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
