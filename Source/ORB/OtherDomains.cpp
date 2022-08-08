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

using namespace CORBA;

namespace Nirvana {

using namespace Core;

namespace ESIOP {

Core::StaticallyAllocated <OtherDomains> OtherDomains::singleton_;

inline
OtherDomain::Reference OtherDomains::get_sync (ProtDomainId domain_id)
{
	OtherDomain::Reference ret;

	auto ins = map_.emplace (domain_id, DEADLINE_MAX);
	if (ins.second) {
		try {
			SYNC_BEGIN (g_core_free_sync_context, &sync_domain_.mem_context ());
			ret = OtherDomain::create (domain_id);
			SYNC_END ();
			if (!ret)
				throw OBJECT_NOT_EXIST ();
			ins.first->second.finish_construction (ret);
		} catch (...) {
			ins.first->second.on_exception ();
			map_.erase (ins.first);
			throw;
		}
	} else {
		ret = ins.first->second.get_if_constructed ();
		if (ret) {
			if (!ret->is_alive (domain_id)) {
				map_.erase (ins.first);
				throw OBJECT_NOT_EXIST ();
			}
		} else
			ret = ins.first->second.get ();
	}
	return ret;
}

OtherDomain::Reference OtherDomains::get (ProtDomainId domain_id)
{
	SYNC_BEGIN (singleton_->sync_domain_, nullptr);
	return singleton_->get_sync (domain_id);
	SYNC_END ();
}

inline
void OtherDomains::housekeeping_sync ()
{
	for (auto it = map_.begin (); it != map_.end ();) {
		OtherDomain::Reference ref = it->second.get_if_constructed ();
		if (ref && ref->ref_cnt_ == 1 && ref->release_time_ <= Chrono::steady_clock () - DELETE_TIMEOUT)
			it = map_.erase (it);
		else
			++it;
	}
}

}
}
