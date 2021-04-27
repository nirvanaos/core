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

#include "ServantProxyBase.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

class ProxyLocal :
	public ServantProxyBase
{
protected:
	ProxyLocal (LocalObject::_ptr_type servant, AbstractBase::_ptr_type abstract_base, ::Nirvana::Core::SyncContext& sync_context) :
		ServantProxyBase (abstract_base, object_ops_, this, sync_context),
		servant_ (servant)
	{}

private:
	static void non_existent_request (ProxyLocal* servant,
		IORequest::_ptr_type call,
		::Nirvana::ConstPointer in_params,
		Unmarshal::_ref_type& unmarshaler,
		::Nirvana::Pointer out_params);

private:
	LocalObject::_ptr_type servant_;

	static const Operation object_ops_ [3];
};

}
}
}

#endif
