// Core implementation of the Runnable interface
#ifndef NIRVANA_CORE_RUNNABLE_H_
#define NIRVANA_CORE_RUNNABLE_H_

#include "CoreInterface.h"

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE Runnable :
	public CoreInterface
{
public:
	virtual void run () = 0;
};

void run_in_neutral_context (Runnable& runnable, CORBA::Nirvana::Interface_ptr environment = CORBA::Nirvana::Interface::_nil ());

}
}

#endif
