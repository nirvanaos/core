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

	static CORBA::Internal::Type <ObjectId>::ABI_ret _s_activate_object (
		CORBA::Internal::Bridge <PortableServer::POA>* _b, Interface* servant,
		Interface* env)
	{
		try {
			return CORBA::Internal::Type <ObjectId>::ret (_implementation (_b).
				activate_object (CORBA::Internal::Type <CORBA::Object>::in (servant)));
		} catch (CORBA::Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
		return CORBA::Internal::Type <ObjectId>::ret ();
	}

	ObjectId activate_object (CORBA::Object::_ptr_type proxy)
	{
		if (active_object_map_.empty ())
			_add_ref ();
		uintptr_t ptr = (uintptr_t)&proxy;
		ObjectId objid ((const CORBA::Octet*)&ptr, (const CORBA::Octet*)(&ptr + 1));
		std::pair <AOM::iterator, bool> ins = active_object_map_.emplace (objid, proxy);
		if (!ins.second)
			throw PortableServer::POA::ServantAlreadyActive ();
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
	typedef ObjectId Key;
	typedef CORBA::Object::_ref_type Val;

	typedef phmap::flat_hash_map <Key, Val, 
		std::hash <Key>, std::equal_to <Key>, Nirvana::Core::UserAllocator
		<std::pair <Key, Val> > > AOM;

	AOM active_object_map_;
};

}
}

#endif
