#include "WaitablePtr.h"
#include <Nirvana/Runnable_s.h>
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

using namespace CORBA;
using namespace CORBA::Nirvana;

class WaitablePtr::WaitForCreation :
	public Servant <WaitablePtr::WaitForCreation, Runnable>,
	public LifeCycleStatic <>
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
	uintptr_t p = ptr_.load ();
	if ((p & STATE_MASK) == PTR_WAITLIST) {
		run_in_neutral_context (&WaitForCreation (*this));
		p = ptr_.load ();
	}
	if (p & STATE_MASK) {
		assert ((p & STATE_MASK) == PTR_EXCEPTION);
		const Exception* pe = (const Exception*)(p & ~STATE_MASK);
		if (pe)
			pe->raise ();
		else
			throw NO_MEMORY ();
	}
	return (void*)p;
}

inline void WaitablePtr::run_in_neutral ()
{
	Thread& thread = Thread::current ();
	ExecDomain* cur = thread.execution_domain ();
	uintptr_t icur = (uintptr_t)cur | PTR_WAITLIST;
	uintptr_t p = ptr_.load ();
	while ((p & STATE_MASK) == PTR_WAITLIST) {
		cur->wait_list_next_ = (ExecDomain*)(p & ~STATE_MASK);
		if (ptr_.compare_exchange_weak (p, icur)) {
			cur->suspend ();
			thread.execution_domain (nullptr);
			break;
		}
	}
}

void WaitablePtr::set_ptr (uintptr_t p)
{
	assert (PTR_WAITLIST != (p & STATE_MASK));
	for (ExecDomain* wait_list = (ExecDomain*)(ptr_.exchange (p) & ~STATE_MASK); wait_list;) {
		ExecDomain* ed = wait_list;
		wait_list = ed->wait_list_next_;
		ed->resume ();
	}
}

void WaitablePtr::set_exception (const Exception& ex)
{
	if (NO_MEMORY ().__code () == ex.__code ())
		set_ptr (PTR_EXCEPTION);
	else {
		try {
			set_ptr ((uintptr_t)ex.__clone () | PTR_EXCEPTION);
		} catch (...) {
			set_ptr (PTR_EXCEPTION);
		}
	}
}

void WaitablePtr::set_unknown_exception ()
{
	try {
		set_ptr (((uintptr_t)new UNKNOWN ()) | PTR_EXCEPTION);
	} catch (...) {
		set_ptr (PTR_EXCEPTION);
	}
}

}
}
