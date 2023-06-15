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
#include "unmarshal_object.h"
#include "../Binder.h"
#include "../ProtDomain.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

Object::_ref_type unmarshal_object (StreamIn& stream, ReferenceRemoteRef& unconfirmed_remote_ref)
{
	IDL::String primary_iid;
	stream.read_string (primary_iid);
	if (primary_iid.empty ())
		return nullptr; // nil reference
	return unmarshal_object (primary_iid, stream, unconfirmed_remote_ref);
}

Object::_ref_type unmarshal_object (Internal::String_in primary_iid, StreamIn & stream, ReferenceRemoteRef & unconfirmed_remote_ref)
{
	Object::_ref_type obj;
	IIOP::ListenPoint listen_point;
	IOP::ObjectKey object_key;
	IOP::TaggedComponentSeq components;
	IOP::TaggedProfileSeq addr;
	stream.read_tagged (addr);
	sort (addr);
	const IOP::TaggedProfile* profile = binary_search (addr, IOP::TAG_INTERNET_IOP);
	if (!profile)
		throw IMP_LIMIT (MAKE_OMG_MINOR (1)); // Unable to use any profile in IOR.
	else {
		Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (profile->profile_data ()));
		IIOP::Version ver;
		stm.read_one (ver);
		if (ver.major () != 1)
			throw NO_IMPLEMENT (MAKE_OMG_MINOR (3));
		stm.read_string (listen_point.host ());
		CORBA::UShort port;
		stm.read_one (port);
		if (stm.other_endian ())
			Nirvana::byteswap (port);
		listen_point.port (port);
		stm.read_seq (object_key);
		if (ver.minor () >= 1) {
			if (components.empty ())
				stm.read_tagged (components);
			else {
				IOP::TaggedComponentSeq comp;
				stm.read_tagged (comp);
				components.insert (components.end (), comp.begin (), comp.end ());
			}
		}
		if (stm.end ())
			throw INV_OBJREF ();
	}

	profile = binary_search (addr, IOP::TAG_MULTIPLE_COMPONENTS);
	if (profile) {
		Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (profile->profile_data ()));
		if (components.empty ())
			stm.read_tagged (components);
		else {
			IOP::TaggedComponentSeq comp;
			stm.read_tagged (comp);
			components.insert (components.end (), comp.begin (), comp.end ());
		}
		if (stm.end ())
			throw INV_OBJREF ();
	}

	sort (components);
	ULong ORB_type = 0;
	auto component = binary_search (components, IOP::TAG_ORB_TYPE);
	if (component) {
		Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (component->component_data ()));
		ORB_type = stm.read32 ();
		if (stm.other_endian ())
			Internal::byteswap (ORB_type);
		if (stm.end ())
			throw INV_OBJREF ();
	}

	size_t host_len = listen_point.host ().size ();
	const Char* host = listen_point.host ().data ();
	if (ORB_type == ESIOP::ORB_TYPE
		&& (0 == host_len ||
			(LocalAddress::singleton ().host ().size () == host_len
				&& std::equal (host, host + host_len, LocalAddress::singleton ().host ().data ())))) {
		// Local Nirvana system

		// Check for SysDomain
		if (ESIOP::is_system_domain ()) {
			Services::Service ss = system_service (object_key);
			if (ss < Services::SERVICE_COUNT)
				obj = Services::bind (ss);
		}
		if (!obj) {
			if (!Nirvana::Core::SINGLE_DOMAIN) {
				// Multiple domain system
				ESIOP::ProtDomainId domain_id;
				if (object_key.size () == 1 && object_key.front () == 0)
					domain_id = ESIOP::sys_domain_id ();
				else
					domain_id = ESIOP::get_prot_domain_id (object_key);
				if (ESIOP::current_domain_id () == domain_id) {
					// Object belongs current domain
					ESIOP::erase_prot_domain_id (object_key);
					if (SysObjectKey <Nirvana::Core::ProtDomain>::equal (object_key))
						obj = Nirvana::Core::ProtDomain::_this ();
					else
						obj = PortableServer::Core::POA_Root::unmarshal (primary_iid, object_key); // Local reference
				} else {
					obj = Binder::unmarshal_remote_reference (domain_id, primary_iid, addr,
						object_key, ORB_type, components, unconfirmed_remote_ref);
				}
			} else {
				// Single domain system
				if (SysObjectKey <Nirvana::Core::ProtDomain>::equal (object_key))
					obj = Nirvana::Core::ProtDomain::_this ();
				else
					obj = PortableServer::Core::POA_Root::unmarshal (primary_iid, object_key); // Local reference
			}
		}
	} else {
		obj = Binder::unmarshal_remote_reference (listen_point, primary_iid, addr,
			object_key, ORB_type, components, unconfirmed_remote_ref);
	}

	return obj;
}

}
}
