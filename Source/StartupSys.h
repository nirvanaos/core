#ifndef NIRVANA_CORE_STARTUPSYS_H_
#define NIRVANA_CORE_STARTUPSYS_H_

#include "Startup.h"

namespace Nirvana {
namespace Core {

class StartupSys : public Startup
{
public:
	static DeadlineTime default_deadline ()
	{
		return INFINITE_DEADLINE;
	}

	StartupSys (int argc, char* argv []) :
		Startup (argc, argv)
	{}

	~StartupSys ()
	{}

	virtual void run ();
};

}
}

#endif
