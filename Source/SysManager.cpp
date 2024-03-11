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
#include "SysManager.h"
#include "SysDomain.h"
#include "ORB/ORB.h"
#include "ORB/ServantProxyObject.h"
//#include <CORBA/Proxy/ProxyBase.h>

using namespace CORBA;

namespace Nirvana {
namespace Core {

void SysManager::shutdown (unsigned flags)
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

Object::_ref_type SysManager::prot_domain_ref (ESIOP::ProtDomainId domain_id)
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
		s += CORBA::Static_the_orb::to_hex (oct >> 4);
		s += CORBA::Static_the_orb::to_hex (oct & 0xf);
	}
	s += "%01";
	return CORBA::Static_the_orb::string_to_object (s, CORBA::Internal::RepIdOf <Nirvana::ProtDomainCore>::id);
}

SysManager& SysManager::get_call_context (Nirvana::SysManager::_ref_type& ref, Ref <SyncContext>& sync)
{
	Core::SysDomain& sd = Core::SysDomain::implementation ();
	ref = sd.provide_manager ();
	CORBA::Core::ServantProxyObject* proxy = CORBA::Core::object2proxy (ref);
	sync = &proxy->sync_context ();
	return *static_cast <SysManager*> (static_cast <Internal::Bridge <PortableServer::ServantBase>*> (&proxy->servant ()));
}

}
}
