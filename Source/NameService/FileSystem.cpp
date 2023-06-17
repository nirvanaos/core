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
#include "FileSystem.h"
#include "../ORB/SysAdapterActivator.h"
#include "FileActivator.h"

using namespace CORBA;
using namespace PortableServer;
using CORBA::Core::Services;
using PortableServer::Core::SysAdapterActivator;
using namespace CosNaming;

namespace Nirvana {

using namespace Core;

namespace FS {
namespace Core {

StaticallyAllocated <POA::_ref_type> FileSystem::adapter_;

FileSystem::FileSystem ()
{
	Roots roots = Port::FileSystem::get_roots ();
	for (auto& r : roots) {
		roots_.emplace (std::move (r.dir), r.factory);
	}

	// Create file system POA
	POA::_ref_type parent = POA::_narrow (Services::bind (Services::RootPOA));
	PolicyList policies;
	policies.push_back (parent->create_lifespan_policy (LifespanPolicyValue::PERSISTENT));
	policies.push_back (parent->create_id_assignment_policy (IdAssignmentPolicyValue::USER_ID));
	policies.push_back (parent->create_request_processing_policy (RequestProcessingPolicyValue::USE_SERVANT_MANAGER));
	POA::_ref_type adapter = parent->create_POA (SysAdapterActivator::filesystem_adapter_name_,
		parent->the_POAManager (), policies);
	adapter->set_servant_manager (make_stateless <FileActivator> ()->_this ());
}

Object::_ref_type FileSystem::get_reference (const ObjectId& id, Internal::String_in iid)
{
	return adapter ()->create_reference_with_id (id, iid);
}

Dir::_ref_type FileSystem::get_dir (const ObjectId& id)
{
	Dir::_ref_type dir = Dir::_narrow (get_reference (id, Internal::RepIdOf <Dir>::id));
	assert (dir);
	return dir;
}

File::_ref_type FileSystem::get_file (const ObjectId& id)
{
	File::_ref_type file = File::_narrow (get_reference (id, Internal::RepIdOf <File>::id));
	assert (file);
	return file;
}

Object::_ref_type FileSystem::get_reference (const ObjectId& id)
{
	return get_reference (id, get_binding_type (id) == BindingType::nobject ?
		Internal::RepIdOf <File>::id : Internal::RepIdOf <Dir>::id);
}

void FileSystem::set_error_number (int err)
{
	ExecDomain::current ().runtime_global_.error_number = err;
}

}
}
}
