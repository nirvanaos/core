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
#ifndef NIRVANA_ORB_CORE_PROXYLOCAL_H_
#define NIRVANA_ORB_CORE_PROXYLOCAL_H_
#pragma once

#include "ServantProxyBase.h"

namespace CORBA {
namespace Core {

/// LocalObject operations servant-side proxy
class ProxyLocal :
	public ServantProxyBase
{
	typedef ServantProxyBase Base;

public:
	typedef CORBA::LocalObject ServantInterface;

	Bridge <Object>* _get_object (Internal::Type <IDL::String>::ABI_in iid, Interface* env) NIRVANA_NOEXCEPT
	{
		return static_cast <Bridge <Object>*> (Base::_s_get_object (this, iid, env));
	}

	/// \returns User LocalObject implementation
	LocalObject::_ptr_type servant () const NIRVANA_NOEXCEPT
	{
		return static_cast <CORBA::LocalObject*> (&Base::servant ());
	}

protected:
	ProxyLocal (LocalObject::_ptr_type servant) :
		ServantProxyBase (servant)
	{}

private:
	virtual Boolean non_existent () override;
	virtual void marshal (StreamOut& out) override;
};

/// Get proxy for local object.
/// 
/// \param obj Local object pointer.
///   Ensure that it is really local object.
/// \returns Proxy pointer.
inline const ProxyLocal* local2proxy (Object::_ptr_type obj) NIRVANA_NOEXCEPT
{
	return static_cast <const ProxyLocal*> (static_cast <Internal::Bridge <Object>*> (&obj));
}

}
}

#endif
