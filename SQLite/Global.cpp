/*
* Nirvana SQLite driver.
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
#include "pch.h"
#include "Global.h"
#include "mem_methods.h"
#include "mutex_methods.h"
#include "filesystem.h"
#include "Activator.h"
#include <Nirvana/System.h>

namespace SQLite {

inline
Global::Global () :
	file_system_ (Nirvana::FileSystem::_narrow (CosNaming::NamingContext::_narrow (
		CORBA::g_ORB->resolve_initial_references ("NameService"))->resolve (CosNaming::Name (1))))
{
	PortableServer::POA::_ref_type root = PortableServer::POA::_narrow (
		CORBA::g_ORB->resolve_initial_references ("RootPOA"));

	if (!file_system_ || !root)
		throw CORBA::INITIALIZE ();

	CORBA::PolicyList policies;
	policies.push_back (root->create_lifespan_policy (PortableServer::LifespanPolicyValue::PERSISTENT));
	policies.push_back (root->create_id_assignment_policy (PortableServer::IdAssignmentPolicyValue::USER_ID));
	policies.push_back (root->create_request_processing_policy (PortableServer::RequestProcessingPolicyValue::USE_SERVANT_MANAGER));
	policies.push_back (root->create_id_uniqueness_policy (PortableServer::IdUniquenessPolicyValue::MULTIPLE_ID));
	PortableServer::POA::_ref_type adapter = root->create_POA ("sqlite", root->the_POAManager (), policies);
	adapter->set_servant_manager (CORBA::make_stateless <Activator> ()->_this ());
	adapter_ = std::move (adapter);

	sqlite3_config (SQLITE_CONFIG_PAGECACHE, nullptr, 0, 0);
	sqlite3_config (SQLITE_CONFIG_MALLOC, &mem_methods);
	sqlite3_config (SQLITE_CONFIG_MUTEX, &mutex_methods);
	sqlite3_vfs_register (&vfs, 1);
	sqlite3_initialize ();
}

inline
Global::~Global ()
{
	sqlite3_shutdown ();
}

const Global global;

Nirvana::Access::_ref_type Global::open_file (const IDL::String& url, uint_fast16_t flags) const
{
	// Get full path name
	CosNaming::Name name;
	Nirvana::g_system->append_path (name, url, true);

	// Open file
	name.erase (name.begin ());
	return file_system_->open (name, flags, 0);
}

}
