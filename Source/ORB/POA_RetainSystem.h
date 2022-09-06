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
#ifndef NIRVANA_ORB_CORE_POA_RETAINSYSTEM_H_
#define NIRVANA_ORB_CORE_POA_RETAINSYSTEM_H_
#pragma once

#include "POA_Retain.h"
#include <Nirvana/Hash.h>

namespace PortableServer {
namespace Core {

/// System-generated object id.
struct ObjectIdSys
{
	void* address;
	typedef CORBA::Core::ReferenceLocal::Timestamp Timestamp;
	Timestamp timestamp;

	ObjectIdSys (CORBA::Core::ReferenceLocal& reference) :
		address (&reference),
		timestamp (reference.timestamp ())
	{}

	size_t hash () const NIRVANA_NOEXCEPT
	{
		return Nirvana::Hash::hash_bytes (this, UNALIGNED_SIZE);
	}

	bool operator == (const ObjectIdSys& rhs) const NIRVANA_NOEXCEPT
	{
		return address == rhs.address && timestamp == rhs.timestamp;
	}

	ObjectId to_object_id () const
	{
		ObjectId oid;
		oid.resize (UNALIGNED_SIZE);
		void** p = (void**)oid.data ();
		*p = address;
		*(Timestamp*)(p + 1) = timestamp;
		return oid;
	}

	static const ObjectIdSys* from_object_id (const ObjectId& oid) NIRVANA_NOEXCEPT
	{
		if (oid.size () == UNALIGNED_SIZE)
			return (const ObjectIdSys*)oid.data ();
		else
			return nullptr;
	}

	static const size_t UNALIGNED_SIZE = sizeof (void*) + sizeof (Timestamp);
};

}
}

namespace std {

template <>
struct hash <PortableServer::Core::ObjectIdSys>
{
	size_t operator () (const PortableServer::Core::ObjectIdSys& sid) const
	{
		return sid.hash ();
	}
};

}

namespace PortableServer {
namespace Core {

/// SYSTEM_ID and RETAIN
class NIRVANA_NOVTABLE POA_RetainSystem : public POA_Retain
{
	typedef POA_Retain Base;

public:
	virtual void destroy (bool etherealize_objects, bool wait_for_completion) override;
	
	// Object activation and deactivation
	virtual ObjectId activate_object (CORBA::Object::_ptr_type p_servant) override;
	virtual void activate_object_with_id (const ObjectId& oid, CORBA::Object::_ptr_type p_servant) override;
	virtual void deactivate_object (const ObjectId& oid) override;

	// Reference creation operations
	virtual CORBA::Object::_ref_type create_reference (const CORBA::RepositoryId& intf) override;
	virtual CORBA::Object::_ref_type create_reference_with_id (const ObjectId& oid, const CORBA::RepositoryId& intf) override;
	
	// Identity mapping operations:
	virtual CORBA::Object::_ref_type id_to_reference (const ObjectId& oid) override;

protected:
	POA_RetainSystem (CORBA::servant_reference <POAManager>&& manager) :
		Base (std::move (manager))
	{}

	virtual void serve (CORBA::Core::RequestInPOA& request, Nirvana::Core::MemContext* memory) override;

	// Active Object Map (AOM)
	typedef ObjectMap <ObjectIdSys> AOM;

	static Nirvana::Core::CoreRef <CORBA::Core::ProxyObject>
		get_proxy (CORBA::Object::_ptr_type p_servant);

	AOM::iterator activate (const ObjectIdSys& oid, Nirvana::Core::CoreRef <CORBA::Core::ProxyObject>&& proxy);
	AOM_Val deactivate (const ObjectId& oid);

protected:
	AOM active_object_map_;
};

class NIRVANA_NOVTABLE POA_RetainSystemUnique : public POA_RetainSystem
{
	typedef POA_RetainSystem Base;

public:
	virtual void destroy (bool etherealize_objects, bool wait_for_completion) override;

	// Object activation and deactivation
	virtual ObjectId activate_object (CORBA::Object::_ptr_type p_servant) override;
	virtual void activate_object_with_id (const ObjectId& oid, CORBA::Object::_ptr_type p_servant) override;
	virtual void deactivate_object (const ObjectId& oid) override;

	// Identity mapping operations:
	virtual ObjectId servant_to_id (CORBA::Object::_ptr_type p_servant) override;
	virtual CORBA::Object::_ref_type servant_to_reference (CORBA::Object::_ptr_type p_servant) override;

protected:
	POA_RetainSystemUnique (CORBA::servant_reference <POAManager>&& manager) :
		Base (std::move (manager))
	{}

	typedef POA_Retain::ServantMap <AOM> ServantMap;

	ServantMap::iterator servant_add (CORBA::Object::_ptr_type p_servant);
	ServantMap::const_iterator servant_find (CORBA::Object::_ptr_type p_servant);

protected:
	ServantMap servant_map_;
};

}
}

#endif
