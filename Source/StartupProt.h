#ifndef NIRVANA_CORE_STARTUPPROT_H_
#define NIRVANA_CORE_STARTUPPROT_H_

#include "Runnable.h"

namespace Nirvana {
namespace Core {

class StartupProt : public ImplStatic <Runnable>
{
public:
	static DeadlineTime default_deadline ()
	{
		return INFINITE_DEADLINE;
	}

	StartupProt (int argc, char* argv [])
	{}

	virtual void run ();
};

}
}

#endif
