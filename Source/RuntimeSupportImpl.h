#ifndef NIRVANA_CORE_RUNTIMESUPPORTIMPL_H_
#define NIRVANA_CORE_RUNTIMESUPPORTIMPL_H_

// Currently we use phmap::flat_hash_map.
// https://github.com/greg7mdp/parallel-hashmap
// See also:
// https://martin.ankerl.com/2019/04/01/hashmap-benchmarks-01-overview/

#include "UserObject.h"
#include "UserAllocator.h"
#include <Nirvana/RuntimeSupport_s.h>
#include "LifeCyclePseudo.h"
#include "CoreInterface.h"
#include "parallel-hashmap/parallel_hashmap/phmap.h"

namespace Nirvana {
namespace Core {

class RuntimeProxyImpl :
	public UserObject, // Allocate from user heap.
	public CORBA::Nirvana::ImplementationPseudo <RuntimeProxyImpl, RuntimeProxy>,
	public LifeCyclePseudo <RuntimeProxyImpl>
{
public:
	RuntimeProxyImpl (const void* obj) :
		object_ (obj)
	{}

	~RuntimeProxyImpl ()
	{}

	const void* object () const
	{
		return object_;
	}

	void remove ()
	{
		assert (object_);
		object_ = nullptr;
	}

private:
	const void* object_;
};

class RuntimeSupportImpl
{
	typedef phmap::flat_hash_map
		<const void*, Core_var <RuntimeProxyImpl>,
		std::hash <const void*>, std::equal_to <const void*>, UserAllocator <std::pair <const void* const, Core_var <RuntimeProxyImpl> > > > ProxyMap;
public:
	RuntimeProxy_var runtime_proxy_get (const void* obj)
	{
		std::pair <ProxyMap::iterator, bool> ins = proxy_map_.insert (ProxyMap::value_type (obj, nullptr));
		if (ins.second) {
			try {
				ins.first->second = Core_var <RuntimeProxyImpl>::create <RuntimeProxyImpl> (obj);
			} catch (...) {
				proxy_map_.erase (ins.first);
				throw;
			}
		}
		return RuntimeProxy::_duplicate (ins.first->second->_get_ptr ());
	}

	void runtime_proxy_remove (const void* obj)
	{
		ProxyMap::iterator f = proxy_map_.find (obj);
		if (f != proxy_map_.end ()) {
			f->second->remove ();
			proxy_map_.erase (f);
		}
	}

	void cleanup ()
	{
		proxy_map_.clear ();
	}

private:
	ProxyMap proxy_map_;
};

}
}

#endif
