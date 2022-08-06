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
#include "OtherDomains.h"
#include "../Synchronized.h"

namespace Nirvana {

using namespace Core;

namespace ESIOP {

Core::StaticallyAllocated <OtherDomains> OtherDomains::singleton_;

inline
WeakPtr <OtherDomain> OtherDomains::get_sync (ProtDomainId domain_id)
{
	WeakPtr <OtherDomain> ret;

retry:
	auto ins = map_.emplace (domain_id, DEADLINE_MAX);
	if (ins.second) {
		try {
			SYNC_BEGIN (g_core_free_sync_context, &sync_domain_.mem_context ());
			ret = WeakPtr <OtherDomain> (OtherDomain::create (domain_id));
			SYNC_END ();
			ins.first->second.finish_construction (ret);
		} catch (...) {
			ins.first->second.on_exception ();
			map_.erase (ins.first);
			throw;
		}
	} else {
		ret = ins.first->second.get_if_constructed ();
		if (ret) {
			CoreRef <OtherDomain> p = ret.get ();
			if (p && !p->is_alive (domain_id)) {
				ret.release ();
				map_.erase (ins.first);
				goto retry;
			}
		} else
			ret = ins.first->second.get ();
	}
	return ret;
}

WeakPtr <OtherDomain> OtherDomains::get (ProtDomainId domain_id)
{
	SYNC_BEGIN (singleton_->sync_domain_, nullptr);
	return singleton_->get_sync (domain_id);
	SYNC_END ();
}

inline
void OtherDomains::housekeeping_sync ()
{
	for (auto it = map_.begin (); it != map_.end (); ++it) {

	}
}

}
}
