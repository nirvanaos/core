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
#ifndef NIRVANA_ORB_CORE_SYSADAPTERACTIVATOR_H_
#define NIRVANA_ORB_CORE_SYSADAPTERACTIVATOR_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/PortableServer_s.h>
#include "Services.h"
#include "../NameService/FileActivator.h"

namespace PortableServer {
namespace Core {

class SysAdapterActivator :
	public CORBA::servant_traits <AdapterActivator>::Servant <SysAdapterActivator>
{
public:
	static void initialize ()
	{
		POA::_ref_type root = POA::_narrow (CORBA::Core::Services::bind (CORBA::Core::Services::RootPOA));
		root->the_activator (CORBA::make_stateless <SysAdapterActivator> ()->_this ());
	}

	static const char filesystem_adapter_name_ [];

	static bool unknown_adapter (POA::_ptr_type parent, const IDL::String& name)
	{
		if (name == filesystem_adapter_name_) {
			CORBA::PolicyList policies;
			policies.push_back (parent->create_lifespan_policy (LifespanPolicyValue::PERSISTENT));
			policies.push_back (parent->create_id_assignment_policy (IdAssignmentPolicyValue::USER_ID));
			policies.push_back (parent->create_request_processing_policy (RequestProcessingPolicyValue::USE_SERVANT_MANAGER));
			POA::_ref_type adapter = parent->create_POA (name, parent->the_POAManager (), policies);
			adapter->set_servant_manager (CORBA::make_stateless <Nirvana::FS::Core::FileActivator> ()->_this ());
			return true;
		}
		return false;
	}
};

}
}

#endif