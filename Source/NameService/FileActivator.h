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
#ifndef NIRVANA_CORE_FILEACTIVATOR_H_
#define NIRVANA_CORE_FILEACTIVATOR_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/PortableServer_s.h>
#include "FileSystem.h"

namespace PortableServer {
namespace Core {

class FileActivator :
	public CORBA::servant_traits <ServantActivator>::Servant <FileActivator>
{
public:
	static void initialize () noexcept
	{
		Nirvana::Core::FileSystem::adapter_.construct ();
	}

	static void terminate () noexcept
	{
		POA::_ref_type adapter = std::move (Nirvana::Core::FileSystem::adapter ());
		if (adapter) {
			try {
				adapter->destroy (true, true);
			} catch (...) {}
		}
	}

	PortableServer::ServantBase::_ref_type incarnate (const ObjectId& id, POA::_ptr_type adapter)
	{
		return Nirvana::Core::FileSystem::incarnate (id);
	}

	void etherealize (const ObjectId& id, POA::_ptr_type adapter,
		CORBA::Object::_ptr_type servant, bool cleanup_in_progress, bool remaining_activations)
	{
		Nirvana::Core::FileSystem::etherealize (id, servant);
	}

	static void create (POA::_ptr_type parent, const IDL::String& name)
	{
		// Create file system POA
		CORBA::PolicyList policies;
		policies.push_back (parent->create_lifespan_policy (LifespanPolicyValue::PERSISTENT));
		policies.push_back (parent->create_id_assignment_policy (IdAssignmentPolicyValue::USER_ID));
		policies.push_back (parent->create_request_processing_policy (RequestProcessingPolicyValue::USE_SERVANT_MANAGER));
		policies.push_back (parent->create_id_uniqueness_policy (IdUniquenessPolicyValue::MULTIPLE_ID));
		POA::_ref_type adapter = parent->create_POA (name, parent->the_POAManager (), policies);
		adapter->set_servant_manager (CORBA::make_stateless <FileActivator> ()->_this ());

		Nirvana::Core::FileSystem::adapter_ = std::move (adapter);
	}

};

}
}

#endif
