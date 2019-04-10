// Nirvana project.
// Protection domain.
#ifndef NIRVANA_CORE_PROTDOMAIN_H_
#define NIRVANA_CORE_PROTDOMAIN_H_

#include "core.h"

#ifdef _WIN32
#include "PortProtDomain.h"
namespace Nirvana {
namespace Core {
typedef Windows::ProtDomainWindows ProtDomainBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class ProtDomain :
	public CoreObject,
	public ProtDomainBase
{
public:
	static void initialize ()
	{
		singleton_ = new ProtDomain;
	}

	static void terminate ()
	{
		delete singleton_;
		singleton_ = nullptr;
	}

	static ProtDomain& singleton ()
	{
		assert (singleton_);
		return *singleton_;
	}

private:
	ProtDomain ()
	{}

private:
	static ProtDomain* singleton_;
};

}
}

#endif
