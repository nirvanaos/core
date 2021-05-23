/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_CORE_RUNTIMESUPPORTIMPL_H_
#define NIRVANA_CORE_RUNTIMESUPPORTIMPL_H_

#include <CORBA/Server.h>
#include <generated/System_s.h>
#include "RuntimeSupport.h"
#include "LifeCyclePseudo.h"
#include "CoreInterface.h"
#include "parallel-hashmap/parallel_hashmap/phmap.h"

namespace Nirvana {
namespace Core {

/// 
/// Currently we use phmap::flat_hash_map:
/// https://github.com/greg7mdp/parallel-hashmap
/// See also:
/// https://martin.ankerl.com/2019/04/01/hashmap-benchmarks-01-overview/
/// 
template <template <class T> class Allocator>
class RuntimeSupportImpl : public RuntimeSupport
{
	class RuntimeProxyImpl :
		public CORBA::Internal::ImplementationPseudo <RuntimeProxyImpl, RuntimeProxy>,
		public LifeCyclePseudo <RuntimeProxyImpl>
	{
	public:
		void* operator new (size_t cb)
		{
			return Allocator <char>::allocate (cb);
		}

		void operator delete (void* p, size_t cb)
		{
			Allocator <char>::deallocate ((char*)p, cb);
		}

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

	typedef phmap::flat_hash_map <const void*, CoreRef <RuntimeProxyImpl>,
		std::hash <const void*>, std::equal_to <const void*>, Allocator <std::pair <const void* const, CoreRef <RuntimeProxyImpl> > > > ProxyMap;
public:
	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj);
	virtual void runtime_proxy_remove (const void* obj);

	void cleanup ()
	{
		proxy_map_.clear ();
	}

private:
	ProxyMap proxy_map_;
};

template <template <class T> class Allocator>
RuntimeProxy::_ref_type RuntimeSupportImpl <Allocator>::runtime_proxy_get (const void* obj)
{
	std::pair <ProxyMap::iterator, bool> ins = proxy_map_.insert (ProxyMap::value_type (obj, nullptr));
	if (ins.second) {
		try {
			ins.first->second = CoreRef <RuntimeProxyImpl>::template create <RuntimeProxyImpl> (obj);
		} catch (...) {
			proxy_map_.erase (ins.first);
			throw;
		}
	}
	return ins.first->second->_get_ptr ();
}

template <template <class T> class Allocator>
void RuntimeSupportImpl <Allocator>::runtime_proxy_remove (const void* obj)
{
	typename ProxyMap::iterator f = proxy_map_.find (obj);
	if (f != proxy_map_.end ()) {
		f->second->remove ();
		proxy_map_.erase (f);
	}
}

}
}

#endif
