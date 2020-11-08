#include "Runnable.h"
#include "ExecContext.h"
#include "Thread.h"
#include <utility>

using namespace std;

namespace Nirvana {
namespace Core {

void run_in_neutral_context (Runnable& runnable)
{
	ExecContext& neutral_context = Thread::current ().neutral_context ();
	neutral_context.run_in_context (runnable);
}

}
}
