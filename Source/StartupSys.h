#ifndef NIRVANA_CORE_STARTUPSYS_H_
#define NIRVANA_CORE_STARTUPSYS_H_

#include "Runnable.h"

namespace Nirvana {
namespace Core {

class StartupSys : public ImplStatic <Runnable>
{
public:
	static DeadlineTime default_deadline ()
	{
		return INFINITE_DEADLINE;
	}

	StartupSys (int argc, char* argv [])
	{}

	~StartupSys ()
	{}

	virtual void run ();
};

}
}

#endif
