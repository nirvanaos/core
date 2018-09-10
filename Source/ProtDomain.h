// Nirvana project.
// Protection domain.
#ifndef NIRVANA_CORE_PROTDOMAIN_H_
#define NIRVANA_CORE_PROTDOMAIN_H_

#include "SyncDomain.h"
#ifdef _WIN32
#include "Windows/ProtDomainBase.h"
namespace Nirvana {
namespace Core {
typedef Windows::ProtDomainBase ProtDomainBase;
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

	void schedule (SyncDomain& sd, bool update)
	{
//		ProtDomainBase::schedule (sd.min_deadline (), &sd, update);
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
