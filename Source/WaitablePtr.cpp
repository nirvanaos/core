#include "WaitablePtr.h"
#include "Runnable.h"
#include "ExecDomain.h"
#include "Thread.h"
#include <CORBA/CORBA.h>

namespace Nirvana {
namespace Core {

using namespace CORBA;
using namespace CORBA::Nirvana;

class NIRVANA_NOVTABLE WaitablePtr::WaitForCreation :
	public Runnable
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
		ImplStatic <WaitForCreation> runnable (std::ref (*this));
		run_in_neutral_context (runnable);
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
	ExecDomain* exec_domain = thread.exec_domain ();
	assert (exec_domain);
	Ptr pcur = Ptr (exec_domain, TAG_WAITLIST);
	Ptr p = load ();
	while (p.tag_bits () == TAG_WAITLIST) {
		exec_domain->wait_list_next_ = (ExecDomain*)(void*)p;
		if (compare_exchange (p, pcur)) {
			exec_domain->suspend ();
			thread.exec_domain (nullptr);
			break;
		}
	}
}

void WaitablePtr::set_ptr (Ptr p)
{
	assert (TAG_WAITLIST != p.tag_bits ());
	for (ExecDomain* wait_list = (ExecDomain*)(void*)exchange (p); wait_list;) {
		ExecDomain* ed = wait_list;
		wait_list = ed->wait_list_next_;
		ed->resume ();
	}
}

void WaitablePtr::set_exception (const Exception& ex)
{
	if (NO_MEMORY ().__code () == ex.__code ())
		set_ptr (Ptr (nullptr, TAG_EXCEPTION));
	else {
		try {
			// Clone exception
			TypeCode_ptr tc = ex.__type_code ();
			size_t cb = sizeof (Exception) + tc->_size ();
			Exception* pex = (Exception*)g_core_heap.allocate (nullptr, cb, 0);
			try {
				*pex = ex;
				tc->_copy (pex->__data (), ex.__data ());
			} catch (...) {
				g_core_heap.release (pex, cb);
				throw;
			}
			set_ptr (Ptr (pex, TAG_EXCEPTION));
		} catch (...) {
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

void WaitablePtr::delete_exception (Exception* pex)
{
	if (pex) {
		TypeCode_ptr tc = pex->__type_code ();
		size_t cb = sizeof (Exception) + tc->_size ();
		pex->~Exception ();
		g_core_heap.release (pex, cb);
	}
}

}
}
