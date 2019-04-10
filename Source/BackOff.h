#ifndef NIRVANA_CORE_BACKOFF_H_
#define NIRVANA_CORE_BACKOFF_H_

#include "PortBackOff.h"

namespace Nirvana {
namespace Core {

class BackOff :
	public PortBackOff
{
public:
	BackOff ()
	{}

	~BackOff ()
	{}

	void sleep ()
	{
		// TODO: Implement some algorithm to choice the sleep time
		PortBackOff::sleep (0);
	}
};

}
}

#endif
