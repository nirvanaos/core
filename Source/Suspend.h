#ifndef NIRVANA_CORE_SUSPEND_H_
#define NIRVANA_CORE_SUSPEND_H_

#include "Runnable.h"

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE Suspend :
	public Runnable
{
public:
	static void suspend () NIRVANA_NOEXCEPT
	{
		ImplStatic <Suspend> runnable;
		run_in_neutral_context (runnable);
	}

public:
	virtual void run ();
};

}
}

#endif
