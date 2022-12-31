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
	CORBA::servant_reference <DomainLocal> get (ProtDomainId domain_id);

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
