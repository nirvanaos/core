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
#include "Domain.h"
#include "../Binder.h"
#include "POA_Root.h"

using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

const Internal::Operation Domain::op_FT_HB = {"FT_HB"};
const Internal::Operation Domain::op_ping = {"ping"};

Domain::Domain (unsigned flags, TimeBase::TimeT request_latency, TimeBase::TimeT heartbeat_interval,
	TimeBase::TimeT heartbeat_timeout) :
	flags_ (flags),
	last_ping_in_time_ (Nirvana::Core::Chrono::steady_clock ()),
	request_latency_ (request_latency),
	heartbeat_interval_ (heartbeat_interval),
	heartbeat_timeout_ (heartbeat_timeout)
{}

Domain::~Domain ()
{}

void Domain::_add_ref () NIRVANA_NOEXCEPT
{
	ref_cnt_.increment ();
}

void Domain::_remove_ref () NIRVANA_NOEXCEPT
{
	if (!ref_cnt_.decrement_seq ()) {
		SyncContext& sc = Binder::singleton ().sync_domain ();
		if (&SyncContext::current () == &sc)
			destroy ();
		else
			GarbageCollector::schedule (*this, sc);
	}
}

void Domain::add_owned_objects (const IDL::Sequence <IOP::ObjectKey>& keys, ReferenceLocalRef* objs)
{
	PortableServer::Core::POA_Root::get_DGC_objects (keys, objs);
	for (ReferenceLocalRef* end = objs + keys.size (); objs != end; ++objs) {
		if (*objs) {
			const IOP::ObjectKey& key = (*objs)->object_key ();
			local_objects_.emplace (key, std::move (*objs));
		}
	}
}

}
}
