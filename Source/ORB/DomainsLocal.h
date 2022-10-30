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
#ifndef NIRVANA_ORB_CORE_DOMAINSLOCAL_H_
#define NIRVANA_ORB_CORE_DOMAINSLOCAL_H_
#pragma once

#include "DomainLocal.h"
#include "../MapUnorderedStable.h"
#include "../WaitableRef.h"
#include "../UserAllocator.h"
#include "../Synchronized.h"
#include <CORBA/Servant_var.h>

namespace ESIOP {

class DomainsLocalWaitable
{
	static const TimeBase::TimeT DEADLINE_MAX = 10 * TimeBase::MILLISECOND;

public:
	CORBA::servant_reference <DomainLocal> get (Nirvana::Core::Service& service, ProtDomainId domain_id)
	{
		auto ins = map_.emplace (domain_id, DEADLINE_MAX);
		if (ins.second) {
			Map::reference entry = *ins.first;
			try {
				Ptr p;
				SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, &service.memory ());
				p.reset (new DomainLocal (std::ref (service), domain_id));
				SYNC_END ();
				PortableServer::Servant_var <DomainLocal> ret (p.get ());
				entry.second.finish_construction (std::move (p));
				return ret;
			} catch (...) {
				entry.second.on_exception ();
				map_.erase (domain_id);
				throw;
			}
		} else
			return ins.first->second.get ().get ();
	}

	void erase (ProtDomainId domain_id)
	{
		map_.erase (domain_id);
	}

private:
	typedef ProtDomainId Key;
	typedef std::unique_ptr <DomainLocal> Ptr;
	typedef Nirvana::Core::WaitableRef <Ptr> Val;

	typedef Nirvana::Core::MapUnorderedStable <Key, Val, std::hash <Key>,
		std::equal_to <Key>, Nirvana::Core::UserAllocator <std::pair <Key, Val> > >
		Map;

	Map map_;
};

class DomainsLocalSimple
{
public:
	CORBA::servant_reference <DomainLocal> get (Nirvana::Core::Service& service, ESIOP::ProtDomainId domain_id)
	{
		return PortableServer::Servant_var <DomainLocal> (&map_.emplace (
			std::piecewise_construct, std::forward_as_tuple (domain_id),
			std::forward_as_tuple (std::ref (service), domain_id)).first->second);
	}

	void erase (ESIOP::ProtDomainId domain_id) NIRVANA_NOEXCEPT
	{
		map_.erase (domain_id);
	}

private:
	typedef ProtDomainId Key;
	typedef DomainLocal Val;

	typedef Nirvana::Core::MapUnorderedStable <Key, Val, std::hash <Key>,
		std::equal_to <Key>, Nirvana::Core::UserAllocator <std::pair <Key, Val> > >
		Map;

	Map map_;
};

using DomainsLocal = std::conditional_t <OtherDomain::slow_creation, DomainsLocalWaitable, DomainsLocalSimple>;

}

#endif
