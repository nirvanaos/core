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
#ifndef NIRVANA_ORB_CORE_POA_RETAIN_H_
#define NIRVANA_ORB_CORE_POA_RETAIN_H_
#pragma once

#include "POA_Base.h"
#include "../MapUnorderedStable.h"
#include "../MapUnorderedUnstable.h"

namespace PortableServer {
namespace Core {

class POA_Retain : public POA_Base
{
	typedef POA_Base Base;

protected:
	POA_Retain (CORBA::servant_reference <POAManager>&& manager) :
		Base (std::move (manager))
	{}

	// Active Object Map (AOM) value.
	class AOM_Val : public Nirvana::Core::CoreRef <CORBA::Core::ProxyObject>
	{
		typedef Nirvana::Core::CoreRef <CORBA::Core::ProxyObject> Base;
	public:
		AOM_Val (Base&& proxy) NIRVANA_NOEXCEPT :
			Base (std::move (proxy))
		{}

		AOM_Val (CORBA::Core::ProxyObject& proxy) NIRVANA_NOEXCEPT :
			Base (&proxy)
		{}

		AOM_Val (AOM_Val&& src) NIRVANA_NOEXCEPT :
			Base (std::move (src))
		{}

		AOM_Val& operator = (AOM_Val&& src) NIRVANA_NOEXCEPT
		{
			Base::operator = (std::move (src));
			return *this;
		}

		~AOM_Val ()
		{
			CORBA::Core::ProxyObject* p = *this;
			if (p)
				p->deactivate ();
		}
	};

	// We always use stable map for AOM, because POA_Activator uses WaitableRef which requires the
	// pointer stability.

	template <class Key>
	using ObjectMap = Nirvana::Core::MapUnorderedStable <Key, AOM_Val, std::hash <Key>,
		std::equal_to <Key>, Nirvana::Core::UserAllocator <std::pair <Key, AOM_Val> > >;

	typedef const PortableServer::ServantBase* UserServantPtr;

	template <class AOM>
	using ServantMap = Nirvana::Core::MapUnorderedUnstable <UserServantPtr, const typename AOM::value_type*, std::hash <UserServantPtr>,
		std::equal_to <UserServantPtr>, Nirvana::Core::UserAllocator <std::pair <UserServantPtr, const typename AOM::value_type*> > >;

	static UserServantPtr get_servant (CORBA::Object::_ptr_type p_servant);
};

}
}

#endif
