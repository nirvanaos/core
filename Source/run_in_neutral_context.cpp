#include "Runnable.h"
#include "ExecContext.h"
#include "Thread.h"
#include <utility>

using namespace std;

namespace Nirvana {
namespace Core {

void run_in_neutral_context (Runnable& runnable)
{
	ExecContext* neutral_context = Thread::current ().neutral_context ();
	assert (neutral_context);
	runnable._add_ref ();
	CORBA::Nirvana::Environment env;
	neutral_context->runnable_ = &runnable;
	neutral_context->environment_ = &env;
	neutral_context->switch_to ();
	env.check ();
}

}
}
