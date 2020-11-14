// Core implementation of the Runnable interface
#ifndef NIRVANA_CORE_RUNNABLE_H_
#define NIRVANA_CORE_RUNNABLE_H_

#include "CoreInterface.h"
#include <exception>

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE Runnable :
	public CoreInterface
{
public:
	virtual void run () = 0;
	virtual void on_exception () NIRVANA_NOEXCEPT;
	virtual void on_crash (Word error_code) NIRVANA_NOEXCEPT;
};

void run_in_neutral_context (Runnable& runnable) NIRVANA_NOEXCEPT;

}
}

#endif
