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
#include "ProtDomains.h"
#include "../Binder.h"

using namespace CORBA;
using namespace Nirvana::Core;

namespace ESIOP {

servant_reference <DomainProt> ProtDomainsWaitable::get (ProtDomainId domain_id)
{
	auto ins = map_.emplace (domain_id, DEADLINE_MAX);
	if (ins.second) {
		typename Map::reference entry = *ins.first;
		try {
			Ptr p;
			SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, &Nirvana::Core::MemContext::current ());
			p.reset (new DomainProt (domain_id));
			SYNC_END ();

			Binder::singleton ().start_domains_housekeeping ();

			PortableServer::Servant_var <CORBA::Core::Domain> ret (p.get ());
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

servant_reference <DomainProt> ProtDomainsWaitable::find (ProtDomainId domain_id) const noexcept
{
	auto it = map_.find (domain_id);
	if (it != map_.end () && it->second.is_constructed ())
		return it->second.get ().get ();
	return nullptr;
}

servant_reference <DomainProt> ProtDomainsSimple::get (ProtDomainId domain_id)
{
	auto ins = map_.emplace (std::piecewise_construct, std::forward_as_tuple (domain_id),
		std::forward_as_tuple (domain_id));
	if (ins.second) {
		try {
			Binder::singleton ().start_domains_housekeeping ();
		} catch (...) {
			map_.erase (ins.first);
			throw;
		}
	}
	return PortableServer::Servant_var <CORBA::Core::Domain> (&ins.first->second);
}

servant_reference <DomainProt> ProtDomainsSimple::find (ProtDomainId domain_id) const NIRVANA_NOEXCEPT
{
	auto it = map_.find (domain_id);
	if (it != map_.end ())
		return &const_cast <DomainProt&> (it->second);
	return nullptr;
}

}
