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
		if (!proxy)
			throw CORBA::BAD_PARAM ();

		if (active_object_map_.empty ()) // TODO: Remove?
			_add_ref ();
		uintptr_t ptr = (uintptr_t)&proxy;
		ObjectId objid ((const CORBA::Octet*)&ptr, (const CORBA::Octet*)(&ptr + 1));
		std::pair <AOM::iterator, bool> ins = active_object_map_.emplace (objid, proxy);
		if (!ins.second)
			throw ServantAlreadyActive ();
		return objid;
	}

	void deactivate_object (const ObjectId& oid)
	{
		if (!active_object_map_.erase (oid))
			throw ObjectNotActive ();
		if (active_object_map_.empty ()) // TODO: Remove?
			_remove_ref ();
	}

	static CORBA::Internal::Type <ObjectId>::ABI_ret _s_servant_to_id (
		CORBA::Internal::Bridge <PortableServer::POA>* _b, Interface* servant,
		Interface* env)
	{
		try {
			return CORBA::Internal::Type <ObjectId>::ret (_implementation (_b).
				servant_to_id (CORBA::Internal::Type <CORBA::Object>::in (servant)));
		} catch (CORBA::Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
		return CORBA::Internal::Type <ObjectId>::ret ();
	}

	ObjectId servant_to_id (CORBA::Object::_ptr_type proxy) const
	{
		for (AOM::const_iterator it = active_object_map_.begin (); it != active_object_map_.end (); ++it) {
			if (&proxy == &CORBA::Object::_ptr_type (it->second))
				return it->first;
		}
		throw ServantNotActive ();
	}

	static Interface* _s_servant_to_reference (
		CORBA::Internal::Bridge <PortableServer::POA>* _b, Interface* servant,
		Interface* env)
	{
		try {
			return CORBA::Internal::Type <CORBA::Object>::ret (_implementation (_b).
				servant_to_reference (CORBA::Internal::Type <CORBA::Object>::in (servant)));
		} catch (CORBA::Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
		return CORBA::Internal::Type <CORBA::Object>::ret ();
	}

	CORBA::Object::_ref_type servant_to_reference (CORBA::Object::_ptr_type proxy) const
	{
		for (AOM::const_iterator it = active_object_map_.begin (); it != active_object_map_.end (); ++it) {
			if (&proxy == &CORBA::Object::_ptr_type (it->second))
				return proxy;
		}
		throw ServantNotActive ();
	}

	static Interface* _s_reference_to_servant (
		CORBA::Internal::Bridge <PortableServer::POA>* _b, Interface* reference,
		Interface* env)
	{
		try {
			return CORBA::Internal::Type <CORBA::Object>::ret (_implementation (_b).
				reference_to_servant (CORBA::Internal::Type <CORBA::Object>::in (reference)));
		} catch (CORBA::Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
		return CORBA::Internal::Type <CORBA::Object>::ret ();
	}

	CORBA::Object::_ref_type reference_to_servant (CORBA::Object::_ptr_type proxy) const
	{
		for (AOM::const_iterator it = active_object_map_.begin (); it != active_object_map_.end (); ++it) {
			if (&proxy == &CORBA::Object::_ptr_type (it->second))
				return proxy;
		}
		throw ObjectNotActive ();
	}

	ObjectId reference_to_id (CORBA::Object::_ptr_type reference) const
	{
		for (AOM::const_iterator it = active_object_map_.begin (); it != active_object_map_.end (); ++it) {
			if (&reference == &CORBA::Object::_ptr_type (it->second))
				return it->first;
		}
		throw WrongAdapter ();
	}

	static Interface* _s_id_to_servant (
		CORBA::Internal::Bridge <PortableServer::POA>* _b, CORBA::Internal::Type <ObjectId>::ABI_in oid,
		Interface* env)
	{
		try {
			return CORBA::Internal::Type <CORBA::Object>::ret (_implementation (_b).
				id_to_servant (CORBA::Internal::Type <ObjectId>::in (oid)));
		} catch (CORBA::Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
		return CORBA::Internal::Type <CORBA::Object>::ret ();
	}

	CORBA::Object::_ref_type id_to_servant (const ObjectId& oid) const
	{
		AOM::const_iterator it = active_object_map_.find (oid);
		if (it != active_object_map_.end ())
			return it->second;

		throw ObjectNotActive ();
	}

	CORBA::Object::_ref_type id_to_reference (const ObjectId& oid) const
	{
		AOM::const_iterator it = active_object_map_.find (oid);
		if (it != active_object_map_.end ())
			return it->second;

		throw ObjectNotActive ();
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
