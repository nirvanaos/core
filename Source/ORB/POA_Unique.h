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
class POA_Unique : 
	public virtual POA_Base,
	public POA_Retain
{
	typedef POA_Retain Base;

public:
	virtual CORBA::servant_reference <CORBA::Core::ProxyObject> deactivate_object (
		const ObjectId& oid) override;

	virtual ObjectId servant_to_id (CORBA::Core::ProxyObject& proxy) override;
	virtual CORBA::Object::_ref_type servant_to_reference (CORBA::Core::ProxyObject& proxy) override;

protected:
	POA_Unique ()
	{}

	POA_Unique (POA_Base* parent, const IDL::String* name, CORBA::servant_reference <POAManager>&& manager) :
		POA_Base (parent, name, std::move (manager))
	{}

	virtual void activate_object (CORBA::Core::ReferenceLocal& ref, CORBA::Core::ProxyObject& proxy) override;

	virtual void destroy_internal (bool etherealize_objects) NIRVANA_NOEXCEPT override;
	virtual void etherealize_objects () NIRVANA_NOEXCEPT override;
	virtual void implicit_deactivate (CORBA::Core::ReferenceLocal& ref,
		CORBA::Core::ProxyObject& proxy) NIRVANA_NOEXCEPT override;

	typedef const CORBA::Core::ProxyObject* ServantPtr;
	typedef CORBA::Core::ReferenceLocal* ReferencePtr;

	ReferencePtr find_servant (const CORBA::Core::ProxyObject& proxy) NIRVANA_NOEXCEPT;

	using ServantMap = Nirvana::Core::MapUnorderedUnstable <ServantPtr, ReferencePtr, std::hash <ServantPtr>,
		std::equal_to <ServantPtr>, Nirvana::Core::UserAllocator <std::pair <ServantPtr, ReferencePtr> > >;

	ServantMap servant_map_;
};

}
}

#endif
