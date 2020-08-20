#include "core.h"
#include "ExecContext.h"
#include "Thread.h"
#include <utility>

using namespace std;

namespace Nirvana {
namespace Core {

Heap* g_core_heap = nullptr;

void run_in_neutral_context (Runnable& runnable)
{
	ExecContext* neutral_context = Thread::current ().neutral_context ();
	assert (neutral_context);
	runnable._add_ref ();
	neutral_context->runnable_ = &runnable;
	neutral_context->switch_to ();
	CORBA::Nirvana::Environment tmp (move (neutral_context->environment ()));
	tmp.check ();
}

}
}
