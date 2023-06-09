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
#ifndef NIRVANA_ORB_CORE_MESSAGING_POLICIES_H_
#define NIRVANA_ORB_CORE_MESSAGING_POLICIES_H_
#pragma once

#include "PolicyImpl.h"
#include <CORBA/PortableServer_s.h>
#include <CORBA/Messaging_s.h>

#define DEFINE_MESSAGING_POLICY(id, Itf, Val, name) DEFINE_POLICY(Messaging::id, Messaging::Itf, Val, name)

namespace CORBA {
namespace Core {

DEFINE_MESSAGING_POLICY (REBIND_POLICY_TYPE, RebindPolicy, Messaging::RebindMode, rebind_mode);
DEFINE_MESSAGING_POLICY (SYNC_SCOPE_POLICY_TYPE, SyncScopePolicy, Messaging::SyncScope, synchronization);
DEFINE_MESSAGING_POLICY (REQUEST_PRIORITY_POLICY_TYPE, RequestPriorityPolicy, Messaging::PriorityRange, priority_range);
DEFINE_MESSAGING_POLICY (REPLY_PRIORITY_POLICY_TYPE, ReplyPriorityPolicy, Messaging::PriorityRange, priority_range);
DEFINE_MESSAGING_POLICY (REQUEST_START_TIME_POLICY_TYPE, RequestStartTimePolicy, TimeBase::UtcT, start_time);
DEFINE_MESSAGING_POLICY (REQUEST_END_TIME_POLICY_TYPE, RequestEndTimePolicy, TimeBase::UtcT, end_time);
DEFINE_MESSAGING_POLICY (REPLY_START_TIME_POLICY_TYPE, ReplyStartTimePolicy, TimeBase::UtcT, start_time);
DEFINE_MESSAGING_POLICY (REPLY_END_TIME_POLICY_TYPE, ReplyEndTimePolicy, TimeBase::UtcT, end_time);
DEFINE_MESSAGING_POLICY (RELATIVE_REQ_TIMEOUT_POLICY_TYPE, RelativeRequestTimeoutPolicy, TimeBase::TimeT, relative_expiry);
DEFINE_MESSAGING_POLICY (RELATIVE_RT_TIMEOUT_POLICY_TYPE, RelativeRoundtripTimeoutPolicy, TimeBase::TimeT, relative_expiry);
DEFINE_MESSAGING_POLICY (ROUTING_POLICY_TYPE, RoutingPolicy, Messaging::RoutingTypeRange, routing_range);
DEFINE_MESSAGING_POLICY (MAX_HOPS_POLICY_TYPE, MaxHopsPolicy, UShort, max_hops);
DEFINE_MESSAGING_POLICY (QUEUE_ORDER_POLICY_TYPE, QueueOrderPolicy, Messaging::Ordering, allowed_orders);

}
}

#undef DEFINE_MESSAGING_POLICY

#endif
