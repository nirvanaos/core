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
#ifndef NIRVANA_ORB_CORE_POA_RETAINUSER_H_
#define NIRVANA_ORB_CORE_POA_RETAINUSER_H_
#pragma once

#include "POA_Retain.h"

namespace PortableServer {
namespace Core {

// UNIQUE_ID, RETAIN
class NIRVANA_NOVTABLE POA_Unique :
	public virtual POA_Retain
{
	typedef POA_Retain Base;

public:
	virtual ObjectId servant_to_id (CORBA::Core::ServantProxyObject& proxy) override;
	virtual CORBA::Object::_ref_type servant_to_reference (CORBA::Core::ServantProxyObject& proxy) override;

protected:
	virtual void activate_object (CORBA::Core::ReferenceLocal& ref,
		CORBA::Core::ServantProxyObject& proxy) override;

	virtual void implicit_deactivate (CORBA::Core::ReferenceLocal& ref,
		CORBA::Core::ServantProxyObject& proxy) noexcept override;
	virtual CORBA::servant_reference <CORBA::Core::ServantProxyObject> deactivate_reference (
		CORBA::Core::ReferenceLocal& ref, bool etherealize, bool cleanup_in_progress) override;

	typedef const CORBA::Core::ServantProxyObject* ServantPtr;
	typedef CORBA::Core::ReferenceLocal* ReferencePtr;

	ReferencePtr find_servant (const CORBA::Core::ServantProxyObject& proxy) noexcept;

	using ServantMap = Nirvana::Core::MapUnorderedUnstable <ServantPtr, ReferencePtr, std::hash <ServantPtr>,
		std::equal_to <ServantPtr>, Nirvana::Core::UserAllocator>;

	ServantMap servant_map_;
};

}
}

#endif
