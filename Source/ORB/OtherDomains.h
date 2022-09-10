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
#ifndef NIRVANA_ORB_CORE_OTHERDOMAINS_H_
#define NIRVANA_ORB_CORE_OTHERDOMAINS_H_
#pragma once

#include "OtherDomain.h"
#include "Services.h"
#include "../MapUnorderedStable.h"
#include "../WaitableRef.h"
#include "../SharedAllocator.h"

namespace Nirvana {
namespace ESIOP {

class OtherDomains :
	public Nirvana::Core::Service
{
	static const TimeBase::TimeT DEADLINE_MAX = 10 * TimeBase::MILLISECOND;
	static const TimeBase::TimeT DELETE_TIMEOUT = 30 * TimeBase::SECOND;

public:
	static OtherDomains& singleton ()
	{
		return static_cast <OtherDomains&> (*CORBA::Core::Services::bind (CORBA::Core::Services::OtherDomains));
	}

	Core::CoreRef <OtherDomain> get (ProtDomainId domain_id);

	static void housekeeping ();

protected:
	OtherDomains ()
	{}

	~OtherDomains ()
	{
		for (auto it = map_.begin (); it != map_.end ();) {
			OtherDomain* p = nullptr;
			try {
				p = it->second.get ();
			} catch (...) {
			}
			delete p;
		}
	}

private:
	void housekeeping_sync ();

private:
	typedef OtherDomain* Ptr;
	typedef Core::WaitableRef <Ptr> WaitableReference;

	typedef Core::MapUnorderedStable <ProtDomainId, WaitableReference, std::hash <ProtDomainId>,
		std::equal_to <ProtDomainId>, Core::SharedAllocator <std::pair <ProtDomainId, WaitableReference> > >
	Map;

	Map map_;
};

}
}

#endif
