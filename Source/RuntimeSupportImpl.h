#ifndef NIRVANA_CORE_RUNTIMESUPPORTIMPL_H_
#define NIRVANA_CORE_RUNTIMESUPPORTIMPL_H_

// Currently we use std::unordered_map.
// TODO: Should be used a more efficient implementation like folly/F14, abseil or EASTL.
// https://martin.ankerl.com/2019/04/01/hashmap-benchmarks-01-overview/
// TODO: The implementation must not allocate memory in the empty container constructor. But std implementation does!
// 
// std::unordered_map depends on std::vector. 
// std::vector can use debug iterators.
// This can cause cyclic dependency.
// So we disable debug iterators for this file.
#pragma push_macro("NIRVANA_DEBUG_ITERATORS")
#undef NIRVANA_DEBUG_ITERATORS
#define NIRVANA_DEBUG_ITERATORS 0

#include "UserObject.h"
#include "UserAllocator.h"
#include <Nirvana/RuntimeSupport_s.h>
#include "ORB/LifeCyclePseudo.h"
#include "CoreInterface.h"
#include <unordered_map>

namespace Nirvana {
namespace Core {

class RuntimeProxyImpl :
	public UserObject, // Allocate from user heap.
	public CORBA::Nirvana::ImplementationPseudo <RuntimeProxyImpl, RuntimeProxy>,
	public CORBA::Nirvana::Core::LifeCyclePseudo <RuntimeProxyImpl>
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
	typedef std::unordered_map
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

#pragma pop_macro("NIRVANA_DEBUG_ITERATORS")

#endif
