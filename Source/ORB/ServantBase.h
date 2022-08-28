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
#ifndef NIRVANA_ORB_CORE_SERVANTBASE_H_
#define NIRVANA_ORB_CORE_SERVANTBASE_H_
#pragma once

#include "CoreImpl.h"
#include "POA_Root.h"

namespace PortableServer {
namespace Core {

/// \brief Core implementation of ServantBase default operations.
class ServantBase final :
	public CORBA::Core::CoreImpl <ServantBase, PortableServer::ServantBase,
		CORBA::Core::ProxyObject>
{
	typedef CORBA::Core::CoreImpl <ServantBase, PortableServer::ServantBase,
		CORBA::Core::ProxyObject> Base;
public:
	using Skeleton <ServantBase, PortableServer::ServantBase>::__get_interface;
	using Skeleton <ServantBase, PortableServer::ServantBase>::__is_a;

	ServantBase (Servant servant) :
		Base (servant)
	{
		timestamp_ = next_timestamp_++;
	}

	// ServantBase default implementation

	Servant _core_servant () NIRVANA_NOEXCEPT
	{
		return &static_cast <PortableServer::ServantBase&> (
			static_cast <CORBA::Internal::Bridge <PortableServer::ServantBase>&> (*this));
	}

	static POA::_ref_type _default_POA ()
	{
		return POA_Root::get_root ();
	}

	bool _non_existent () const
	{
		return false;
	}

	static void initialize () NIRVANA_NOEXCEPT
	{
		next_timestamp_ = (int)(Nirvana::Core::Chrono::UTC ().time () / TimeBase::SECOND);
	}

private:
	static POA* default_POA_;
	static std::atomic <int> next_timestamp_;
};

}
}

#endif
