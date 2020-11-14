#ifndef NIRVANA_LEGACY_CORE_RUNTIMESUPPORTLEGACY_H_
#define NIRVANA_LEGACY_CORE_RUNTIMESUPPORTLEGACY_H_

#include "../RuntimeSupportImpl.h"
#include <mutex> // TODO: Replace with own implementation.

namespace Nirvana {
namespace Legacy {
namespace Core {

class RuntimeSupportLegacy :
	public Nirvana::Core::RuntimeSupportImpl
{
public:
	RuntimeProxy_var runtime_proxy_get (const void* obj)
	{
		std::lock_guard <std::mutex> lock (mutex_);
		return Nirvana::Core::RuntimeSupportImpl::runtime_proxy_get (obj);
	}

	void runtime_proxy_remove (const void* obj)
	{
		std::lock_guard <std::mutex> lock (mutex_);
		Nirvana::Core::RuntimeSupportImpl::runtime_proxy_remove (obj);
	}

private:
	std::mutex mutex_;
};

}
}
}

#endif
