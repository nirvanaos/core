/// \file Supported policies definition
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
#ifndef NIRVANA_ORB_CORE_PORTABLESERVER_POLICIES_H_
#define NIRVANA_ORB_CORE_PORTABLESERVER_POLICIES_H_
#pragma once

#include "PolicyImpl.h"
#include "../CORBA/PortableServer_s.h"
#include "../CORBA/Messaging_s.h"

#define DEFINE_PORTABLE_SERVER_POLICY(id, Itf) DEFINE_POLICY(PortableServer::id, PortableServer::Itf, PortableServer::Itf##Value, value)

namespace CORBA {
namespace Core {

// PortableServer policies

DEFINE_PORTABLE_SERVER_POLICY (THREAD_POLICY_ID, ThreadPolicy);
DEFINE_PORTABLE_SERVER_POLICY (LIFESPAN_POLICY_ID, LifespanPolicy);
DEFINE_PORTABLE_SERVER_POLICY (ID_UNIQUENESS_POLICY_ID, IdUniquenessPolicy);
DEFINE_PORTABLE_SERVER_POLICY (ID_ASSIGNMENT_POLICY_ID, IdAssignmentPolicy);
DEFINE_PORTABLE_SERVER_POLICY (IMPLICIT_ACTIVATION_POLICY_ID, ImplicitActivationPolicy);
DEFINE_PORTABLE_SERVER_POLICY (SERVANT_RETENTION_POLICY_ID, ServantRetentionPolicy);
DEFINE_PORTABLE_SERVER_POLICY (REQUEST_PROCESSING_POLICY_ID, RequestProcessingPolicy);

}
}

#undef DEFINE_PORTABLE_SERVER_POLICY

#endif
