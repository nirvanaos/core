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

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

Object::_ref_type unmarshal_object (StreamIn& stream, ReferenceRemoteRef& unconfirmed_remote_ref)
{
	Object::_ref_type obj;
	{
		IDL::String primary_iid;
		stream.read_string (primary_iid);
		if (primary_iid.empty ())
			return nullptr; // nil reference

		IIOP::ListenPoint listen_point;
		IOP::ObjectKey object_key;
		bool object_key_found = false;
		IOP::TaggedComponentSeq components;
		IOP::TaggedProfileSeq addr;
		stream.read_tagged (addr);
		for (const IOP::TaggedProfile& profile : addr) {
			switch (profile.tag ()) {
			case IOP::TAG_INTERNET_IOP: {
				Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (profile.profile_data ()));
				IIOP::Version ver;
				stm.read (alignof (IIOP::Version), sizeof (IIOP::Version), &ver);
				if (ver.major () != 1)
					throw NO_IMPLEMENT (MAKE_OMG_MINOR (3));
				stm.read_string (listen_point.host ());
				CORBA::UShort port;
				stm.read (alignof (CORBA::UShort), sizeof (CORBA::UShort), &port);
				if (stm.other_endian ())
					Nirvana::byteswap (port);
				listen_point.port (port);
				stm.read_seq (object_key);
				object_key_found = true;
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
			} break;

			case IOP::TAG_MULTIPLE_COMPONENTS: {
				Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (profile.profile_data ()));
				if (components.empty ())
					stm.read_tagged (components);
				else {
					IOP::TaggedComponentSeq comp;
					stm.read_tagged (comp);
					components.insert (components.end (), comp.begin (), comp.end ());
				}
				if (stm.end ())
					throw INV_OBJREF ();
			} break;
			}
		}

		if (!object_key_found)
			throw IMP_LIMIT (MAKE_OMG_MINOR (1)); // Unable to use any profile in IOR.

		sort (components);
		ULong ORB_type = 0;
		auto p = binary_search (components, IOP::TAG_ORB_TYPE);
		if (p) {
			Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (p->component_data ()));
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
#ifdef NIRVANA_SINGLE_DOMAIN
			obj = PortableServer::Core::POA_Root::unmarshal (primary_iid, object_key); // Local reference
#else
			ESIOP::ProtDomainId domain_id;
			auto pc = binary_search (components, ESIOP::TAG_DOMAIN_ADDRESS);
			if (pc) {
				Nirvana::Core::ImplStatic <StreamInEncap> stm (std::ref (pc->component_data ()));
				stm.read (alignof (ESIOP::ProtDomainId), sizeof (ESIOP::ProtDomainId), &domain_id);
				if (stm.other_endian ())
					Internal::byteswap (domain_id);
				if (stm.end ())
					throw INV_OBJREF ();
			} else
				domain_id = ESIOP::sys_domain_id ();
			if (ESIOP::current_domain_id () == domain_id)
				obj = PortableServer::Core::POA_Root::unmarshal (primary_iid, object_key); // Local reference
			else
				obj = Binder::unmarshal_remote_reference (domain_id, primary_iid, addr,
					object_key, ORB_type, components, unconfirmed_remote_ref);
#endif
		} else
			obj = Binder::unmarshal_remote_reference (listen_point, primary_iid, addr,
				object_key, ORB_type, components, unconfirmed_remote_ref);
	}
	return obj;
}

}
}
