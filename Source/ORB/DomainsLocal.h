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
#include <CORBA/Servant_var.h>

namespace ESIOP {

template <template <class> class Al>
class DomainsLocalWaitable
{
	static const TimeBase::TimeT DEADLINE_MAX = 10 * TimeBase::MILLISECOND;

public:
	CORBA::servant_reference <DomainLocal> get (ProtDomainId domain_id);

	CORBA::servant_reference <DomainLocal> find (ProtDomainId domain_id)
	{
		auto it = map_.find (domain_id);
		if (it != map_.end ())
			return it->second.get ().get ();
		else
			return nullptr;
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
		std::equal_to <Key>, Al> Map;

	Map map_;
};

template <template <class> class Al>
inline CORBA::servant_reference <DomainLocal> DomainsLocalWaitable <Al>::get (ProtDomainId domain_id)
{
	auto ins = map_.emplace (domain_id, DEADLINE_MAX);
	if (ins.second) {
		typename Map::reference entry = *ins.first;
		try {
			Ptr p;
			SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, &Nirvana::Core::MemContext::current ());
			p.reset (new DomainLocal (domain_id));
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

template <template <class> class Al>
class DomainsLocalSimple
{
public:
	CORBA::servant_reference <DomainLocal> get (ProtDomainId domain_id)
	{
		return PortableServer::Servant_var <DomainLocal> (&map_.emplace (
			std::piecewise_construct, std::forward_as_tuple (domain_id),
			std::forward_as_tuple (domain_id)).first->second);
	}

	void erase (ProtDomainId domain_id) NIRVANA_NOEXCEPT
	{
		map_.erase (domain_id);
	}

	CORBA::servant_reference <DomainLocal> find (ProtDomainId domain_id) NIRVANA_NOEXCEPT
	{
		auto it = map_.find (domain_id);
		if (it != map_.end ())
			return &it->second;
		return nullptr;
	}

private:
	typedef ProtDomainId Key;
	typedef DomainLocal Val;

	typedef Nirvana::Core::MapUnorderedStable <Key, Val, std::hash <Key>,
		std::equal_to <Key>, Al> Map;

	Map map_;
};

template <template <class> class Al>
using DomainsLocal = std::conditional_t < OtherDomain::slow_creation, DomainsLocalWaitable <Al>, DomainsLocalSimple <Al> > ;

}

#endif
