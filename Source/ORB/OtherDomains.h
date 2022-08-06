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
#include "../MapUnorderedStable.h"
#include "../WeakPtr.h"
#include "../WaitableRef.h"
#include "../SharedAllocator.h"

namespace Nirvana {
namespace ESIOP {

class OtherDomains
{
	static const DeadlineTime DEADLINE_MAX = 10 * System::MILLISECOND;
	static const Core::Chrono::Duration DELETE_TIMEOUT = 30 * System::SECOND;

public:
	OtherDomains () :
		sync_domain_ (std::ref (static_cast <Core::SyncContextCore&> (Core::g_core_free_sync_context)),
			std::ref (static_cast <Core::MemContextCore&> (Core::g_shared_mem_context)))
	{}

	static Core::WeakPtr <OtherDomain> get (ProtDomainId domain_id);

	static void housekeeping ();

	static void initialize ()
	{
		singleton_.construct ();
	}

	static void terminate () NIRVANA_NOEXCEPT
	{
		singleton_.destruct ();
	}

private:
	Core::WeakPtr <OtherDomain> get_sync (ProtDomainId domain_id);
	void housekeeping_sync ();

private:
	typedef Core::WaitableRef <Core::WeakPtr <OtherDomain> > Reference;

	typedef Core::MapUnorderedStable <ProtDomainId, Reference, std::hash <ProtDomainId>,
		std::equal_to <ProtDomainId>, Core::SharedAllocator <std::pair <ProtDomainId, Reference> > >
	Map;

	Core::ImplStatic <Core::SyncDomainImpl> sync_domain_;
	Map map_;

	static Core::StaticallyAllocated <OtherDomains> singleton_;
};

}
}

#endif
