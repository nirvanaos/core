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
#include <unordered_map>
#include "HashOctetSeq.h"
#include "../StaticallyAllocated.h"
#include "IDL/PortableServer_s.h"

namespace PortableServer {
namespace Core {

// Active Object Map (AOM)
typedef ObjectId Key;
typedef CORBA::Object::_ref_type Val;

typedef std::unordered_map <Key, Val,
	std::hash <Key>, std::equal_to <Key>, Nirvana::Core::UserAllocator
	<std::pair <Key, Val> > > AOM;

class POA_Impl :
	public CORBA::Internal::Bridge <PortableServer::POA>
{
public:
	static bool implicit_activation (PortableServer::POA::_ptr_type poa)
	{
		const POA_Impl* p = static_cast <const POA_Impl*> (
			static_cast <CORBA::Internal::Bridge <PortableServer::POA>*> (&poa));
		return p && p->implicit_activation ();
	}

protected:
	POA_Impl (const CORBA::Internal::Bridge <PortableServer::POA>::EPV& epv) :
		CORBA::Internal::Bridge <PortableServer::POA> (epv)
	{}

	virtual bool implicit_activation () const NIRVANA_NOEXCEPT = 0;
};

}
}

namespace CORBA {
namespace Internal {

template <class S>
class InterfaceImpl <S, PortableServer::POA> :
	public PortableServer::Core::POA_Impl,
	public Skeleton <S, PortableServer::POA>
{
protected:
	InterfaceImpl () :
		PortableServer::Core::POA_Impl (Skeleton <S, PortableServer::POA>::epv_)
	{}
};

}
}

namespace PortableServer {
namespace Core {

class POA_Root :
	public CORBA::servant_traits <PortableServer::POA>::Servant <POA_Root>
{
public:
	POA_Root ()
	{}

	~POA_Root ()
	{}

	void destroy (bool etherealize_objects, bool wait_for_completion)
	{
		active_object_map_.clear ();
	}

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

		auto servant = object2servant_core (proxy);
		POA::_ptr_type activated_POA = servant->activated_POA ();
		if (activated_POA && activated_POA->_is_equivalent (_this ()))
			throw ServantAlreadyActive (); // This servant already activated in this POA

		ObjectId oid;
		AOM::const_iterator it = AOM_insert (proxy, oid);

		if (!activated_POA)
			servant->activate (_this (), it->first, true);
		
		return it->first;
	}

	void deactivate_object (const ObjectId& oid)
	{
		auto it = active_object_map_.find (oid);
		if (it == active_object_map_.end ())
			throw ObjectNotActive ();
		auto servant = object2servant_core (it->second);
		POA::_ptr_type activated_POA = servant->activated_POA ();
		assert (activated_POA);
		if (activated_POA && activated_POA->_is_equivalent (_this ()))
			servant->deactivate ();
		active_object_map_.erase (it);
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

	ObjectId servant_to_id (CORBA::Object::_ptr_type proxy)
	{
		if (!proxy)
			throw CORBA::BAD_PARAM ();

		auto servant = object2servant_core (proxy);
		POA::_ptr_type activated_POA = servant->activated_POA ();
		if (activated_POA && activated_POA->_is_equivalent (_this ()))
			return *servant->activated_id ();

		ObjectId oid;
		AOM::const_iterator it = AOM_insert (proxy, oid);
		if (!activated_POA)
			servant->activate (_this (), it->first, true);
		return oid;
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

	CORBA::Object::_ref_type servant_to_reference (CORBA::Object::_ptr_type proxy)
	{
		if (!proxy)
			throw CORBA::BAD_PARAM ();

		auto servant = object2servant_core (proxy);
		POA::_ptr_type activated_POA = servant->activated_POA ();
		if (!activated_POA) {
			ObjectId oid;
			AOM::const_iterator it = AOM_insert (proxy, oid);
			servant->activate (_this (), it->first, true);
		}
		return proxy;
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

	CORBA::Object::_ref_type reference_to_servant (CORBA::Object::_ptr_type proxy)
	{
		if (!proxy)
			throw CORBA::BAD_PARAM ();

		auto servant = object2servant_core (proxy);
		POA::_ptr_type activated_POA = servant->activated_POA ();
		if (!activated_POA)
			throw ObjectNotActive ();
		if (!activated_POA->_is_equivalent (_this ()))
			throw WrongAdapter ();
		return proxy;
	}

	ObjectId reference_to_id (CORBA::Object::_ptr_type proxy)
	{
		auto servant = object2servant_core (proxy);
		POA::_ptr_type activated_POA = servant->activated_POA ();
		if (!activated_POA || !activated_POA->_is_equivalent (_this ()))
			throw WrongAdapter ();
		return *servant->activated_id ();
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

protected:
	virtual bool implicit_activation () const NIRVANA_NOEXCEPT
	{
		return true;
	}

	// Generate unique object id and insert
	AOM::const_iterator AOM_insert (CORBA::Object::_ptr_type proxy, ObjectId& objid);

private:
	AOM active_object_map_;
};

}
}

#endif
