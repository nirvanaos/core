#ifndef NIRVANA_CORE_BACKOFF_H_
#define NIRVANA_CORE_BACKOFF_H_

#include <Port/BackOff.h>

namespace Nirvana {
namespace Core {

class BackOff :
	private Port::BackOff
{
public:
	BackOff ()
	{}

	~BackOff ()
	{}

	void sleep ()
	{
		// TODO: Implement some algorithm to choice the sleep time
		Port::BackOff::sleep (0);
	}
};

}
}

#endif
