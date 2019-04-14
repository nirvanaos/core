// Nirvana project.
// Protection domain.
#ifndef NIRVANA_CORE_PROTDOMAIN_H_
#define NIRVANA_CORE_PROTDOMAIN_H_

#include "core.h"
#include <Port/ProtDomain.h>

namespace Nirvana {
namespace Core {

class ProtDomain :
	public CoreObject,
	private Port::ProtDomain
{
public:
	Port::ProtDomain& port ()
	{
		return *this;
	}

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
