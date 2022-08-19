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
#include "PolicyFactory.h"
#include "PortableServer_policies.h"

#define DEFINE_CREATOR(id) { id, PolicyImpl <id>::create_policy }

namespace CORBA {
namespace Core {

const PolicyFactory::Creator PolicyFactory::creators [SUPPORTED_CNT] = {
	DEFINE_CREATOR(PortableServer::THREAD_POLICY_ID),
	DEFINE_CREATOR (PortableServer::LIFESPAN_POLICY_ID),
	DEFINE_CREATOR (PortableServer::ID_UNIQUENESS_POLICY_ID),
	DEFINE_CREATOR (PortableServer::ID_ASSIGNMENT_POLICY_ID),
	DEFINE_CREATOR (PortableServer::IMPLICIT_ACTIVATION_POLICY_ID),
	DEFINE_CREATOR (PortableServer::SERVANT_RETENTION_POLICY_ID),
	DEFINE_CREATOR (PortableServer::REQUEST_PROCESSING_POLICY_ID)
};

}
}
