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
#ifndef SQLITE_DRIVER_H_
#define SQLITE_DRIVER_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/File.h>
#include "Activator.h"
#include <fnctl.h>

namespace SQLite {

class Driver : public CORBA::servant_traits <NDBC::Driver>::Servant <Driver>
{
public:
	Driver () :
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
	}

	NDBC::DataSource::_ref_type getDataSource (const IDL::String& url) const
	{
		// Create file if not exists
		Nirvana::File::_ref_type file = open_file (url, O_CREAT)->file ();

		// Get file ID and create reference
		return NDBC::DataSource::_narrow (
			adapter_->create_reference_with_id (file->id (), NDBC::_tc_DataSource->id ()));
	}

	Nirvana::FileSystem::_ptr_type file_system () const
	{
		return file_system_;
	}

	Nirvana::Access::_ref_type open_file (const IDL::String& url, uint_fast16_t flags) const;

private:
	Nirvana::FileSystem::_ref_type file_system_;
	PortableServer::POA::_ref_type adapter_;
};

}

#endif
