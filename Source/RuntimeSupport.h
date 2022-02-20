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
#ifndef NIRVANA_CORE_RUNTIMESUPPORT_H_
#define NIRVANA_CORE_RUNTIMESUPPORT_H_
#pragma once

#include <CORBA/Server.h>
#include "IDL/System_s.h"
#include "LifeCyclePseudo.h"
#include "UserObject.h"
#include "UserAllocator.h"
#include "CoreInterface.h"

/// 
/// Currently we use phmap::flat_hash_map:
/// https://github.com/greg7mdp/parallel-hashmap
/// See also:
/// https://martin.ankerl.com/2019/04/01/hashmap-benchmarks-01-overview/
/// 
#include "parallel-hashmap/parallel_hashmap/phmap.h"

namespace Nirvana {
namespace Core {

/// Implements System::runtime_proxy_get() and System::runtime_proxy_remove().
class RuntimeSupport
{
	/// Implements RuntimeProxy interface.
	class RuntimeProxyImpl :
		public CORBA::Internal::ImplementationPseudo <RuntimeProxyImpl, RuntimeProxy>,
		public LifeCyclePseudo <RuntimeProxyImpl>,
		public UserObject
	{
	public:
		RuntimeProxyImpl (const void* obj) NIRVANA_NOEXCEPT :
			object_ (obj)
		{}

		~RuntimeProxyImpl ()
		{}

		const void* object () const NIRVANA_NOEXCEPT
		{
			return object_;
		}

		void remove () NIRVANA_NOEXCEPT
		{
			assert (object_);
			object_ = nullptr;
		}

	private:
		const void* object_;
	};

	typedef phmap::flat_hash_map <const void*, CoreRef <RuntimeProxyImpl>,
		std::hash <const void*>, std::equal_to <const void*>, UserAllocator <std::pair <const void* const, CoreRef <RuntimeProxyImpl>>>> ProxyMap;
public:
	RuntimeProxy::_ref_type runtime_proxy_get (const void* obj)
	{
		auto ins = proxy_map_.emplace (obj, nullptr);
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

	void runtime_proxy_remove (const void* obj) NIRVANA_NOEXCEPT
	{
		auto f = proxy_map_.find (obj);
		if (f != proxy_map_.end ()) {
			f->second->remove ();
			proxy_map_.erase (f);
		}
	}

	void clear () NIRVANA_NOEXCEPT
	{
		ProxyMap tmp;
		proxy_map_.swap (tmp);
	}

private:
	ProxyMap proxy_map_;
};

}
}

#endif
