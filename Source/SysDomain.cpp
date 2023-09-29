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
#include "pch.h"
#include "SysDomain.h"
#include "ORB/Services.h"
#include <CORBA/IOP.h>
#include <CORBA/Proxy/ProxyBase.h>
#include "Binder.h"
#include "ORB/ORB.h"

using namespace CORBA;
using namespace CORBA::Core;
using namespace CORBA::Internal;
using namespace PortableServer;

namespace Nirvana {
namespace Core {

Object::_ref_type create_SysDomain ()
{
	if (ESIOP::is_system_domain ())
		return make_reference <SysDomain> ()->_this ();
	else
		return CORBA::Core::ORB::string_to_object (
			"corbaloc::1.1@/%00", CORBA::Internal::RepIdOf <Nirvana::SysDomainCore>::id);
}

void SysDomain::get_call_context (Ref <SysDomain>& impl, Ref <SyncContext>& sync)
{
	assert (ESIOP::is_system_domain ());
	Object::_ref_type obj = Services::bind (Services::SysDomain);
	impl = static_cast <SysDomain*> (get_implementation (obj));
	const ServantProxyLocal* proxy = get_proxy (obj);
	sync = &proxy->sync_context ();
}

void SysDomain::shutdown (unsigned flags)
{
	if (shutdown_started_)
		return;
	shutdown_started_ = true;
	try {
		if (domains_.empty ())
			Scheduler::shutdown ();
		else
			domains_.shutdown ();
	} catch (...) {
		shutdown_started_ = false;
	}
}

Object::_ref_type SysDomain::prot_domain_ref (ESIOP::ProtDomainId domain_id)
{
	const char prefix [] = "corbaloc::1.1@/";
	IDL::String s;
	s.reserve (std::size (prefix) - 1 + (sizeof (ESIOP::ProtDomainId) + 1) * 3);
	s = prefix;
	ESIOP::ProtDomainId id = domain_id;
	if (ESIOP::PLATFORMS_ENDIAN_DIFFERENT && endian::native == endian::little)
		Nirvana::byteswap (id);
	for (const uint8_t* p = (const uint8_t*)&id, *end = p + sizeof (ESIOP::ProtDomainId); p != end; ++p) {
		s += '%';
		uint8_t oct = *p;
		s += CORBA::Core::ORB::to_hex (oct >> 4);
		s += CORBA::Core::ORB::to_hex (oct & 0xf);
	}
	s += "%01";
	return CORBA::Core::ORB::string_to_object (s, CORBA::Internal::RepIdOf <Nirvana::ProtDomainCore>::id);
}

}
}

