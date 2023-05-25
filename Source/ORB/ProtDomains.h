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

namespace ESIOP {

class ProtDomainsWaitable
{
	static const TimeBase::TimeT DEADLINE_MAX = 10 * TimeBase::MILLISECOND;

public:
	CORBA::servant_reference <DomainProt> get (ProtDomainId domain_id);

	CORBA::servant_reference <DomainProt> find (ProtDomainId domain_id) const noexcept
	{
		auto it = map_.find (domain_id);
		if (it != map_.end ())
			return it->second.get ().get ();
		else
			return nullptr;
	}

	void erase (ProtDomainId domain_id) noexcept
	{
		map_.erase (domain_id);
	}

private:
	typedef ProtDomainId Key;
	typedef std::unique_ptr <DomainProt> Ptr;
	typedef Nirvana::Core::WaitableRef <Ptr> Val;

	typedef Nirvana::Core::MapUnorderedStable <Key, Val, std::hash <Key>,
		std::equal_to <Key>, Nirvana::Core::BinderMemory::Allocator> Map;

	Map map_;
};

class ProtDomainsSimple
{
public:
	CORBA::servant_reference <DomainProt> get (ProtDomainId domain_id);

	void erase (ProtDomainId domain_id) NIRVANA_NOEXCEPT
	{
		map_.erase (domain_id);
	}

	CORBA::servant_reference <DomainProt> find (ProtDomainId domain_id) const NIRVANA_NOEXCEPT
	{
		auto it = map_.find (domain_id);
		if (it != map_.end ())
			return &const_cast <DomainProt&> (it->second);
		return nullptr;
	}

private:
	typedef ProtDomainId Key;

	typedef Nirvana::Core::MapUnorderedStable <Key, DomainProt, std::hash <Key>,
		std::equal_to <Key>, Nirvana::Core::BinderMemory::Allocator> Map;

	Map map_;
};

class ProtDomainsDummy
{
public:
	CORBA::servant_reference <DomainProt> get (ProtDomainId domain_id)
	{
		NIRVANA_UNREACHABLE_CODE ();
		return nullptr;
	}

	void erase (ProtDomainId domain_id) NIRVANA_NOEXCEPT
	{
		NIRVANA_UNREACHABLE_CODE ();
	}

	CORBA::servant_reference <DomainProt> find (ProtDomainId domain_id) const NIRVANA_NOEXCEPT
	{
		NIRVANA_UNREACHABLE_CODE ();
		return nullptr;
	}
};

using ProtDomains = std::conditional_t <Nirvana::Core::SINGLE_DOMAIN, ProtDomainsDummy,
std::conditional_t <OtherDomain::slow_creation, ProtDomainsWaitable, ProtDomainsSimple> >;

}

#endif
