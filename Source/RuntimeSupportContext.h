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
#ifndef NIRVANA_CORE_RUNTIMESUPPORTCONTEXT_H_
#define NIRVANA_CORE_RUNTIMESUPPORTCONTEXT_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/System_s.h>
#include "LifeCyclePseudo.h"
#include "UserObject.h"
#include "UserAllocator.h"
#include <Port/config.h>
#include "MapUnorderedUnstable.h"
#include <memory>

namespace Nirvana {
namespace Core {

class RuntimeSupportContextReal
{
public:
	RuntimeProxy::_ref_type proxy_get (const void* obj)
	{
		auto ins = proxy_map_.emplace (obj, nullptr);
		if (ins.second) {
			try {
				ins.first->second = Ref <RuntimeProxyImpl>::template create <RuntimeProxyImpl> (obj);
			} catch (...) {
				proxy_map_.erase (ins.first);
				throw;
			}
		}
		return ins.first->second->_get_ptr ();
	}

	void proxy_remove (const void* obj) noexcept
	{
		auto f = proxy_map_.find (obj);
		if (f != proxy_map_.end ()) {
			f->second->remove ();
			proxy_map_.erase (f);
		}
	}

private:
	/// Implements RuntimeProxy interface.
	class RuntimeProxyImpl :
		public CORBA::Internal::ImplementationPseudo <RuntimeProxyImpl, RuntimeProxy>,
		public LifeCyclePseudo <RuntimeProxyImpl>,
		public UserObject
	{
	public:
		RuntimeProxyImpl (const void* obj) noexcept :
			object_ (obj)
		{}

		~RuntimeProxyImpl ()
		{}

		const void* object () const noexcept
		{
			return object_;
		}

		void remove () noexcept
		{
			assert (object_);
			object_ = nullptr;
		}

	private:
		const void* object_;
	};

private:
	typedef MapUnorderedUnstable <const void*, Ref <RuntimeProxyImpl>,
		std::hash <const void*>, std::equal_to <const void*>, UserAllocator> ProxyMap;
	ProxyMap proxy_map_;
};


class RuntimeSupportFake
{
public:
	static RuntimeProxy::_ref_type proxy_get (const void* obj)
	{
		return RuntimeProxy::_nil ();
	}

	static void proxy_remove (const void* obj) noexcept
	{}

};

typedef std::conditional <RUNTIME_SUPPORT_DISABLE, RuntimeSupportFake, RuntimeSupportContextReal>::type RuntimeSupportContext;

}
}

#endif
