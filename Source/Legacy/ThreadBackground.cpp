#include "ThreadBackground.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

using namespace Nirvana::Core;

ThreadBackground::ThreadBackground (Nirvana::Core::Runnable& runnable, CORBA::Nirvana::Interface* environment)
{
	Core_var <ExecDomain> exec_domain = ExecDomain::get ();
	execution_domain (exec_domain);
	_add_ref ();
	try {
		ExecDomain::start (exec_domain, runnable, INFINITE_DEADLINE, nullptr, environment, [this]() {this->create (); });
	} catch (...) {
		execution_domain (nullptr);
		_remove_ref ();
		throw;
	}
}

}
}
}
