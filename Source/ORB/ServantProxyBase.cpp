#include "ServantProxyBase.h"
#include "../Runnable.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

using namespace ::Nirvana::Core;

class ServantProxyBase::GarbageCollector :
	public CoreObject,
	public Runnable
{
public:
	GarbageCollector (Interface_ptr servant) :
		servant_ (servant)
	{}

	~GarbageCollector ()
	{}

	void run ()
	{
		release (servant_);
	}

private:
	Interface_ptr servant_;
};

ServantProxyBase::ServantProxyBase (AbstractBase_ptr servant, 
	const Operation object_ops [3], void* object_impl) :
	ProxyManager (Skeleton <ServantProxyBase, IOReference>::epv_, 
		Skeleton <ServantProxyBase, Object>::epv_, primary_interface_id (servant), 
		object_ops, object_impl),
	servant_ (servant),
	ref_cnt_ (0),
	sync_context_ (&SynchronizationContext::current ())
{
	// Fill implementation pointers
	for (InterfaceEntry* ie = interfaces ().begin (); ie != interfaces ().end (); ++ie) {
		if (!ie->implementation) {
			if (!(ie->implementation = servant->_query_interface (ie->iid)))
				throw OBJ_ADAPTER (); // Implementation not found. TODO: Log
		}
	}
}

void ServantProxyBase::add_ref_1 ()
{
	interface_duplicate (&servant_);
}

::Nirvana::Core::RefCounter::UIntType ServantProxyBase::_remove_ref ()
{
	::Nirvana::Core::RefCounter::IntType cnt = ref_cnt_.decrement ();
	if (!cnt) {
		try {
			run_garbage_collector (Core_var <Runnable>::create <ImplDynamic <GarbageCollector> > (servant_));
		} catch (...) {
			// Async call failed, maybe resources are exausted.
			// Fallback to collect garbage in current thread.
			::Nirvana::Core::Synchronized sync (*sync_context_);
			release (servant_);
			// Swallow exception
		}
	} else if (cnt < 0) {
		// TODO: Log error
		ref_cnt_.increment ();
		cnt = 0;
	}
		
	return cnt;
}

}
}
}
