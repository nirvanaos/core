#ifndef NIRVANA_CORE_STARTUPPROT_H_
#define NIRVANA_CORE_STARTUPPROT_H_

#include "Startup.h"

namespace Nirvana {
namespace Core {

class StartupProt : public Startup
{
public:
	static DeadlineTime default_deadline ()
	{
		return INFINITE_DEADLINE;
	}

	StartupProt (int argc, char* argv []) :
		Startup (argc, argv)
	{}

	~StartupProt ()
	{}

	virtual void run ();
};

}
}

#endif
