#include "core.h"
#include "Heap.h"
#include "ExecContext.h"
#include "Thread.h"

namespace Nirvana {

HeapFactory_ptr g_heap_factory;

namespace Core {

Memory_ptr g_core_heap = Memory_ptr::nil ();

void run_in_neutral_context (Runnable_ptr runnable)
{
	ExecContext* neutral_context = Thread::current ().neutral_context ();
	assert (neutral_context);
	neutral_context->runnable_ = Runnable::_duplicate (runnable);
	neutral_context->switch_to ();
	::std::auto_ptr < ::CORBA::Exception> exception (neutral_context->environment ().detach ());
	if (exception.get ())
		exception->_raise ();
}

}
}
