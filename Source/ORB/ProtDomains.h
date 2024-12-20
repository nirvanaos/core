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
#ifndef NIRVANA_ORB_CORE_PROTDOMAINS_H_
#define NIRVANA_ORB_CORE_PROTDOMAINS_H_
#pragma once

#include "DomainProt.h"
#include "../MapUnorderedStable.h"
#include "../WaitableRef.h"
#include <CORBA/Servant_var.h>

namespace CORBA {
namespace Core {

class ProtDomainsWaitable
{
	static const TimeBase::TimeT DEADLINE_MAX = 10 * TimeBase::MILLISECOND;

public:
	ProtDomainsWaitable ()
	{}

	~ProtDomainsWaitable ()
	{
		for (const auto& el : map_) {
			delete el.second.get_if_constructed ();
		}
	}

	servant_reference <DomainProt> get (ESIOP::ProtDomainId domain_id);
	servant_reference <DomainProt> find (ESIOP::ProtDomainId domain_id) const noexcept;

	void erase (ESIOP::ProtDomainId domain_id) noexcept
	{
		Map::iterator it = map_.find (domain_id);
		assert (it != map_.end ());
		delete it->second.get_if_constructed ();
		map_.erase (it);
	}

	bool housekeeping (const TimeBase::TimeT& cur_time) noexcept
	{
		for (auto it = map_.begin (); it != map_.end ();) {
			DomainProt* d = it->second.get_if_constructed ();
			if (d && d->is_garbage (cur_time)) {
				delete d;
				it = map_.erase (it);
			} else
				++it;
		}
		return !map_.empty ();
	}

	void shutdown () noexcept
	{
		for (auto it = map_.begin (); it != map_.end (); ++it) {
			DomainProt* d = it->second.get_if_constructed ();
			if (d)
				d->shutdown ();
		}
	}

private:
	typedef ESIOP::ProtDomainId Key;
	typedef Nirvana::Core::WaitableRef <DomainProt*> Val;

	typedef Nirvana::Core::MapUnorderedStable <Key, Val, std::hash <Key>,
		std::equal_to <Key>, Nirvana::Core::BinderMemory::Allocator> Map;

	Map map_;
};

class ProtDomainsSimple
{
public:
	CORBA::servant_reference <DomainProt> get (ESIOP::ProtDomainId domain_id);
	CORBA::servant_reference <DomainProt> find (ESIOP::ProtDomainId domain_id) const noexcept;

	void erase (ESIOP::ProtDomainId domain_id) noexcept
	{
		map_.erase (domain_id);
	}

	bool housekeeping (const TimeBase::TimeT& cur_time) noexcept
	{
		for (auto it = map_.begin (); it != map_.end ();) {
			if (it->second.is_garbage (cur_time))
				it = map_.erase (it);
			else
				++it;
		}
		return !map_.empty ();
	}

	void shutdown () noexcept
	{
		for (auto it = map_.begin (); it != map_.end (); ++it) {
			it->second.shutdown ();
		}
	}

private:
	typedef ESIOP::ProtDomainId Key;

	typedef Nirvana::Core::MapUnorderedStable <Key, DomainProt, std::hash <Key>,
		std::equal_to <Key>, Nirvana::Core::BinderMemory::Allocator> Map;

	Map map_;
};

class ProtDomainsDummy
{
public:
	CORBA::servant_reference <DomainProt> get (int domain_id)
	{
		NIRVANA_UNREACHABLE_CODE ();
		return nullptr;
	}

	void erase (int domain_id) noexcept
	{
		NIRVANA_UNREACHABLE_CODE ();
	}

	CORBA::servant_reference <DomainProt> find (int domain_id) const noexcept
	{
		NIRVANA_UNREACHABLE_CODE ();
		return nullptr;
	}

	bool housekeeping (const TimeBase::TimeT& cur_time) noexcept
	{
		return false;
	}

	void shutdown () noexcept
	{}
};

using ProtDomains = std::conditional_t <Nirvana::Core::SINGLE_DOMAIN, ProtDomainsDummy,
	std::conditional_t <ESIOP::OtherDomain::slow_creation, ProtDomainsWaitable, ProtDomainsSimple> >;

}
}

#endif
