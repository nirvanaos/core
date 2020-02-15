#include "WaitablePtr.h"
#include <Nirvana/Runnable_s.h>
#include "ExecDomain.h"
#include <CORBA/CORBA.h>

namespace Nirvana {
namespace Core {

using namespace CORBA;
using namespace CORBA::Nirvana;

class WaitablePtr::WaitForCreation :
	public Servant <WaitablePtr::WaitForCreation, Runnable>,
	public LifeCycleStatic
{
public:
	WaitForCreation (WaitablePtr& ptr) :
		ptr_ (ptr)
	{}

	void run ()
	{
		ptr_.run_in_neutral ();
	}

private:
	WaitablePtr& ptr_;
};

void* WaitablePtr::wait ()
{
	Ptr p = load ();
	if (p.tag_bits () == TAG_WAITLIST) {
		WaitForCreation runnable (*this);
		run_in_neutral_context (runnable._get_ptr ());
		p = load ();
	}
	if (p.tag_bits ()) {
		assert (p.tag_bits () == TAG_EXCEPTION);
		const Exception* pe = (const Exception*)static_cast <void*> (p);
		if (pe)
			pe->_raise ();
		else
			throw NO_MEMORY ();
	}
	return (void*)p;
}

inline void WaitablePtr::run_in_neutral ()
{
	Thread& thread = Thread::current ();
	ExecDomain* exec_domain = thread.execution_domain ();
	assert (exec_domain);
	Ptr pcur = Ptr (exec_domain, TAG_WAITLIST);
	Ptr p = load ();
	while (p.tag_bits () == TAG_WAITLIST) {
		static_cast <StackElem&> (*exec_domain).next = p;
		if (compare_exchange (p, pcur)) {
			exec_domain->suspend ();
			thread.execution_domain (nullptr);
			break;
		}
	}
}

void WaitablePtr::set_ptr (Ptr p)
{
	assert (TAG_WAITLIST != p.tag_bits ());
	for (ExecDomain* wait_list = (ExecDomain*)(void*)exchange (p); wait_list;) {
		ExecDomain* ed = wait_list;
		wait_list = reinterpret_cast <ExecDomain*> (static_cast <StackElem&> (*ed).next);
		ed->resume ();
	}
}

void WaitablePtr::set_exception (const Exception& ex)
{
	if (NO_MEMORY ().__code () == ex.__code ())
		set_ptr (Ptr (nullptr, TAG_EXCEPTION));
	else {
		Octet* pex = nullptr;
		try {
			set_ptr (Ptr (ex.__clone (), TAG_EXCEPTION));
		} catch (...) {
			delete [] pex;
			set_ptr (Ptr (nullptr, TAG_EXCEPTION));
		}
	}
}

void WaitablePtr::set_unknown_exception ()
{
	try {
		set_ptr (Ptr (new UNKNOWN (), TAG_EXCEPTION));
	} catch (...) {
		set_ptr (Ptr (nullptr, TAG_EXCEPTION));
	}
}

}
}
