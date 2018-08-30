#ifndef NIRVANA_CORE_BACKOFF_H_
#define NIRVANA_CORE_BACKOFF_H_

#ifdef _WIN32
#include "Windows/win32.h"
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class BackOff
{
public:
	BackOff ()
	{}

	~BackOff ()
	{}

	void sleep ()
	{
#ifdef _WIN32
		::Sleep (0);
#endif
	}
};

}
}

#endif
