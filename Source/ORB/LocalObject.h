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
#ifndef NIRVANA_ORB_CORE_LOCALOBJECT_H_
#define NIRVANA_ORB_CORE_LOCALOBJECT_H_

#include "CoreImpl.h"
#include "ProxyLocal.h"

namespace CORBA {
namespace Internal {
namespace Core {

/// \brief Core implementation of LocalObject default operations.
class LocalObject final :
	public CoreImpl <LocalObject, CORBA::LocalObject, ProxyLocal>
{
	typedef CoreImpl <LocalObject, CORBA::LocalObject, ProxyLocal> Base;
public:
	LocalObject (CORBA::LocalObject::_ptr_type servant, AbstractBase::_ptr_type abstract_base, ::Nirvana::Core::SyncContext& sync_context = ::Nirvana::Core::SyncContext::current ()) :
		Base (servant, abstract_base, std::ref (sync_context))
	{}

	// LocalObject default implementation

	Boolean _non_existent () const
	{
		return false;
	}
};

}
}
}

#endif
