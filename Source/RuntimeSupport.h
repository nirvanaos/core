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
#include <signal.h>
#include <Nirvana/System_s.h>
#include "LifeCyclePseudo.h"
#include "UserObject.h"
#include "UserAllocator.h"
#include "CoreInterface.h"
#include <Port/config.h>
#include "MapUnorderedUnstable.h"

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

	typedef MapUnorderedUnstable <const void*, Ref <RuntimeProxyImpl>,
		std::hash <const void*>, std::equal_to <const void*>, UserAllocator <
		std::pair <const void* const, Ref <RuntimeProxyImpl> > >
	> ProxyMap;

public:
	RuntimeProxy::_ref_type runtime_proxy_get (const void* obj);
	void runtime_proxy_remove (const void* obj) NIRVANA_NOEXCEPT;

	void clear () NIRVANA_NOEXCEPT;

	RuntimeSupport ();
	~RuntimeSupport ();

private:
	ProxyMap proxy_map_;
};

class RuntimeSupportFake
{
public:
	RuntimeProxy::_ref_type runtime_proxy_get (const void* obj)
	{
		return RuntimeProxy::_nil ();
	}

	void runtime_proxy_remove (const void* obj) NIRVANA_NOEXCEPT
	{}

	void clear () NIRVANA_NOEXCEPT
	{}
};

typedef std::conditional <RUNTIME_SUPPORT_DISABLE, RuntimeSupportFake, RuntimeSupport>::type RuntimeSupportImpl;

}
}

#endif
