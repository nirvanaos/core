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
#ifndef NIRVANA_ORB_CORE_POA_H_
#define NIRVANA_ORB_CORE_POA_H_
#pragma once

#include <CORBA/Server.h>
#include "ServantBase.h"
#include "IDL/PortableServer_s.h"
#include "../parallel-hashmap/parallel_hashmap/phmap.h"
#include "HashOctetSeq.h"
#include "../StaticallyAllocated.h"

namespace PortableServer {
namespace Core {

class POA :
	public CORBA::servant_traits <PortableServer::POA>::Servant <POA>
{
public:
	POA ()
	{}

	~POA ()
	{}

	ObjectId activate_object (PortableServer::ServantBase::_ptr_type p_servant)
	{
		if (active_object_map_.empty ())
			_add_ref ();

		uintptr_t ptr = (uintptr_t)&p_servant;
		ObjectId objid ((const CORBA::Octet*)&ptr, (const CORBA::Octet*)(&ptr + 1));
		CORBA::Object::_ptr_type obj = servant2object (p_servant);
		std::pair <AOM::iterator, bool> ins = active_object_map_.emplace (objid, AOM_Val());
		if (!ins.second)
			throw PortableServer::POA::ServantAlreadyActive ();
		ins.first->second.servant = p_servant;
		ins.first->second.object = obj;
		return objid;
	}

	void deactivate_object (const ObjectId& oid)
	{
		if (!active_object_map_.erase (oid))
			throw PortableServer::POA::ObjectNotActive ();
		if (active_object_map_.empty ())
			_remove_ref ();
	}

private:
	// Active Object Map (AOM)
	typedef ObjectId AOM_Key;
	struct AOM_Val
	{
		CORBA::Object::_ref_type object;
		PortableServer::ServantBase::_ref_type servant;
	};

	typedef phmap::flat_hash_map <AOM_Key, AOM_Val,
		std::hash <AOM_Key>, std::equal_to <AOM_Key>, Nirvana::Core::UserAllocator
		<std::pair <AOM_Key, AOM_Val> > > AOM;

	AOM active_object_map_;
};

}
}

#endif
